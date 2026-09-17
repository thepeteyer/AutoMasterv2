# AutoMaster -- Development Notes

This file is for anyone building, extending, or debugging AutoMaster. If
you just want to use the plugin, see [README.md](README.md) instead.

## Status

Incremental MVP. The full architecture described below is wired end-to-end
and all six tabs are functional. This has not been compiled in the
authoring environment (no CMake/JUCE/MSVC toolchain available there -- see
"Building"). Treat it as a strong first pass to compile, audition, and
iterate on.

## Signal chain

```
Input & Gain Staging -> EQ -> Compression -> Saturation -> Stereo Imaging -> Limiter/Loudness
```

Every stage reads its parameters live from the plugin's
`AudioProcessorValueTreeState` (APVTS) every block -- manual knob turns,
host automation, and the AI's "Reset to suggestion" writes all go through
the exact same path. `Freeze` captures a snapshot of that state and
processes from the snapshot instead, so nothing (not even automation) can
move the sound until you unfreeze.

## Transport modes

| Mode | Behaviour |
|---|---|
| **Learn** | Passthrough; accumulates up to 30s of input into a ring buffer for analysis. |
| **Bypass** | Pure passthrough, no analysis, no processing -- reference A/B. |
| **Apply** | The chain is live. Leaving Learn (by pressing any other mode) triggers analysis of whatever was buffered, derives a suggested chain, and writes it into the parameters before switching to Apply. |
| **Freeze** | Chain still runs, but from a captured parameter snapshot; adaptive re-analysis stops. |

**Continuous/Adaptive** (off by default) keeps feeding the ring buffer while
in Apply and re-analyses every ~8 seconds, nudging parameters toward the new
suggestion by `sensitivity * 0.3` per tick rather than snapping.

## Folder structure

```
AutoMaster/
├── CMakeLists.txt
├── scripts/build.ps1 | build.sh
└── Source/
    ├── PluginProcessor.h/.cpp   -- transport state machine, Learn ring buffer,
    │                               APVTS <-> ChainParameters glue, state save/restore
    ├── PluginEditor.h/.cpp      -- top-level layout: transport bar + meters + tabs
    ├── Params.h/.cpp            -- every parameter ID + the APVTS layout
    ├── analysis/
    │   ├── AnalysisTypes.h          -- AnalysisResult (the feature vector)
    │   ├── KWeighting.h              -- ITU-R BS.1770 K-weighting biquad pair
    │   ├── RunningLoudnessMeter.h    -- real-time LUFS-ish meter (auto gain match, limiter target)
    │   └── AnalysisEngine.h/.cpp     -- offline analysis pass (FFT bands, LUFS, crest, correlation, transients)
    ├── rules/
    │   ├── ChainParameters.h    -- the suggested-chain struct, one sub-struct per tab
    │   └── RulesEngine.h/.cpp   -- IRulesEngine interface + DefaultRulesEngine heuristics
    ├── dsp/
    │   ├── Saturator.h          -- stateless waveshaping functions (Tape/Tube/Transistor/Clip)
    │   ├── StereoImager.h       -- 3-band Mid/Side width via cascaded Linkwitz-Riley filters
    │   └── MasteringChain.h/.cpp -- owns and orders all 6 stages, smoothing, metering, latency
    └── gui/
        ├── TransportBar.h/.cpp     -- Learn/Bypass/Apply/Freeze + adaptive controls + status line
        ├── Meters.h/.cpp           -- In/Out LUFS, peak, correlation, auto-gain-trim readout
        ├── StageTabs.h/.cpp        -- generic Knob/Toggle/Choice controls + the 6 concrete tabs
        └── TabbedChainEditor.h/.cpp -- the JUCE TabbedComponent hosting the 6 tabs
```

`RulesEngine::derive(AnalysisResult) -> ChainParameters` is the one interface
boundary meant for a future ML model: swap `DefaultRulesEngine` for a class
that runs a small trained model over the same `AnalysisResult` feature
vector, with zero changes to `PluginProcessor`, `MasteringChain`, or the UI.

## UI structure

Every tab in `gui/StageTabs.cpp` follows the same shape, on purpose:

- A bypass toggle (top-left) and "Reset to AI Suggestion" button (top-right)
  in the same position on every tab.
- One large **hero** knob -- the single macro control a non-expert can use
  without touching anything else (`Tilt`, `Glue`, `Drive`, `Width`,
  `Punch vs Loud`; Gain Staging's hero is `Input Trim` since it only has
  the one meaningful knob).
- A collapsed-by-default **Advanced** disclosure holding the detailed,
  engineer-facing knobs (frequencies, Q, ratio, attack/release, crossovers,
  ceiling, etc).
- Every `KnobControl`/`ToggleControl`/`ChoiceControl` carries a plain-language
  tooltip (shown via the single `juce::TooltipWindow` owned by
  `PluginEditor`) and, for knobs, the parameter's unit is forced into the
  slider's text box via `Slider::setTextValueSuffix` so dB/Hz/ms/LUFS/%
  always show regardless of host/OS text-formatting quirks.
- Each tab has a fixed accent colour (`TabColours` in `StageTabs.h`), used
  for its hero knob's tooltip context and for tinting its tab-bar button in
  `TabbedChainEditor`, so the six stages stay visually distinguishable.

## Building

Same pattern as the sibling NeoSat project:

```bash
# Windows (PowerShell)
pwsh scripts/build.ps1

# macOS / Linux
./scripts/build.sh
```

or manually:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

JUCE 8.0.4 is fetched automatically; pass `-DAUTOMASTER_JUCE_PATH=/path/to/JUCE`
to use a local checkout instead. Output: `build/AutoMaster_artefacts/Release/VST3/AutoMaster.vst3`
(and a Standalone app for quick auditioning). CI builds all three platforms
via `.github/workflows/build.yml`.

## Fixed: EQ tab silencing all audio (ProcessorDuplicator coefficient bug)

**Symptom:** the EQ tab produced silence (or otherwise broken output)
whenever it was actively processing; toggling its Bypass switch on
restored normal sound.

**Root cause:** `juce::dsp::ProcessorDuplicator<Filter, Coefficients>`
shares a single `Coefficients` object between its own `.state` pointer and
every per-channel filter it owns internally -- that link is made exactly
once, inside `prepare()`, by copying the pointer. `MasteringChain::processEq()`
and `processSat()` were updating filters with:

```cpp
eqLowShelf.state = juce::dsp::IIR::Coefficients<float>::makeLowShelf(...);
```

which only repoints the *duplicator's own* `state` member to a brand-new
object -- it does not reach the per-channel filters that actually run
`process()`, since they captured the *old* pointer at `prepare()` time and
never see the new one. Every block, real processing kept running against
whatever coefficients existed when the plugin was first prepared (an
unconfigured default), while every subsequent knob/AI-suggested value was
silently discarded. Bypass wasn't inverted -- `isBypassed()` correctly
skips the whole function when the toggle is on, which is exactly why
enabling Bypass "fixed" it: it stopped calling the broken code path at all.

**Fix:** mutate the shared object's contents in place instead of repointing
it, so every already-bound per-channel filter sees the update immediately:

```cpp
*eqLowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(...);
```

Applied to all four EQ bands and both saturation tone-tilt filters (same
bug pattern). Also added a Nyquist clamp on every EQ frequency and a Q
clamp before calling the `make*` coefficient factories, so an edge-case
sample rate or automated value can't push a frequency past Nyquist and
produce NaN/Inf that would otherwise poison the filter's IIR state
indefinitely (until the next `reset()`).

**Verifying it (manual test -- no automated harness in this environment):**

1. Build and load AutoMaster in a host or the Standalone app.
2. Feed it a steady tone or pink noise.
3. Open the EQ tab, leave Bypass off, and confirm all knobs are at their
   defaults (all gains at 0 dB, Tilt at 0).
4. Output level should match input level to within ~0.1 dB (a flat EQ is a
   unity-gain pass-through) and should sound identical to input, not
   silent or distorted.
5. Turn the Tilt knob or an individual band gain and confirm the tone
   audibly brightens/darkens/changes rather than cutting out.
6. Toggle Bypass on/off and confirm both states pass audio -- Bypass
   should sound identical to the dry source; Bypass-off-at-defaults should
   also sound like the dry source (unity gain), and only differ once you
   move a knob off its default.

If you add an automated test suite later, encode step 3-4 as: process a
known buffer through `MasteringChain` with all EQ params at their APVTS
defaults and assert `output ≈ input` within a small epsilon.

## Known simplifications (read before trusting a mix decision to this)

- **LUFS is ungated.** True ITU-R BS.1770-4 integrated loudness requires
  absolute (-70 LUFS) and relative (-10 LU) gating over 400ms blocks; this
  implementation K-weights and mean-squares the whole captured buffer with no
  gating. Close enough for "does this feel loud" heuristics, not for a
  loudness-compliance report.
- **`juce::dsp::Compressor`/`Limiter` gain-reduction isn't exposed** by JUCE,
  so the GR meters are an RMS/peak-delta approximation around each stage,
  display-only -- they don't feed back into the DSP.
- **Stereo crossovers are minimum-phase IIR** (Linkwitz-Riley), so they add
  no reported latency but do add some phase shift. The spec's "linear-phase
  optional" upgrade would need an FIR crossover and `setLatencySamples()`
  to account for it.
- **The only reported plugin latency is the limiter's true-peak
  oversampler** (2x, `juce::dsp::Oversampling`). Nothing else in the chain
  adds latency in this build.
- **Transient density and spectral tilt are simple proxies**, not a genre
  classifier: an onset-rate count and a linear-regression slope over 6 bands.
  There's no genre detection at all -- the rules engine works purely from
  the numeric feature vector.
- **Not compiled here.** No CMake/JUCE toolchain was available in this
  environment; the code is written carefully against the JUCE 8 API but
  hasn't been build-verified. Expect a first-compile pass to surface a
  handful of fixable issues.

## Deliberately deferred (not in this MVP pass)

- Reference-track spectral overlay (EQ tab).
- A dedicated multiband-dynamics 7th tab.
- Genre-aware target curves (the rules engine is genre-agnostic; it reacts
  only to measured features).
- A trained ML model behind `IRulesEngine` (the seam exists; nothing occupies it yet).
- Full undo/redo for knob changes (host/DAW undo generally covers parameter
  changes already; a dedicated in-plugin undo stack was left out for this pass).
- A user preset browser beyond what the host's own plugin-state save/recall gives you.
