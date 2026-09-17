# AutoMaster

**AutoMaster listens to your song, figures out what it needs, and builds you
a starting master automatically** -- EQ, compression, warmth, stereo width,
and final loudness -- so you're tweaking a finished-sounding mix instead of
staring at a blank plugin. You can use its suggestion as-is, nudge a few
big knobs, or dig as deep as you want. It's a VST3/Standalone plugin built
with JUCE.

> Looking to build the plugin from source, or curious how it works under
> the hood? See [DEVELOPMENT.md](DEVELOPMENT.md). This file is just about
> using it.

---

## Quick start

1. **Load AutoMaster** on your master bus (or a stereo mix bus).
2. Press **Learn** and play 15-30 seconds of the loudest, most representative
   part of your song.
3. Press **Apply** (or just press any other mode button -- it applies
   automatically). AutoMaster analyses what it heard and builds a starting
   master.
4. **Tweak** -- turn the one big knob on each tab (see below) to taste.
5. Happy with it? Press **Freeze** so nothing can accidentally change the
   sound from here.
6. **Export/bounce** your track as usual.

That's the whole workflow. Everything below just explains each piece in
more depth.

---

## The transport bar: Learn / Bypass / Apply / Freeze

This row at the top is the main control -- it decides what AutoMaster is
doing at any given moment.

- **Learn** -- AutoMaster listens to whatever you play, without changing
  the sound at all, so it can figure out what your track needs. Play the
  loudest chorus or most "full" section of the song, not the quiet intro --
  it needs at least a few seconds, and 15-30 seconds gives it a much better
  picture than 2 seconds does.
- **Bypass** -- everything is switched off; you hear your track completely
  untouched. Use this to A/B against the processed sound and make sure
  you're actually making things better.
- **Apply** -- the mastering chain is active and doing its thing. Switching
  out of Learn into any other mode automatically triggers the analysis and
  loads its suggestion in for you, so most of the time you'll go
  Learn → Apply directly.
- **Freeze** -- locks in whatever the knobs are currently set to. Nothing --
  not your host's automation, not an accidental mouse bump -- can change the
  sound while frozen. Use this once you're happy and about to export, so a
  stray click doesn't undo your work.

**Continuous / Adaptive** (off by default) makes AutoMaster keep listening
in the background while it's in Apply mode and slowly nudge its settings
every several seconds as the song changes, instead of deciding once and
sticking with it. The **Sensitivity** knob controls how strongly it nudges
-- low is barely noticeable, high reacts more. Most people can leave this
off; it's there for tracks with very different sections (quiet verse, huge
chorus) where a single one-time analysis might not fit the whole song.

---

## The six tabs

Every tab follows the same layout: a **bypass switch** and a
**"Reset to AI Suggestion"** button in the same top corners every time, one
large knob in the middle that does most of the work, and an **Advanced**
section (collapsed by default) with the finer, engineer-style controls if
you want to go deeper. Hover any knob for a plain-language explanation.

### 1. Gain Staging
Sets the input level before anything else touches it. **Input Trim** is the
main knob -- turn it if the source feels too quiet or too hot going in.
**Auto Gain Match** (in Advanced) keeps the volume matched between Bypass
and Apply so your A/B comparisons are about tone, not just "louder sounds
better."

### 2. EQ
Shapes the overall tone. **Tilt** is the main knob: turn right for a
brighter mix, left for something warmer and darker. The Advanced section
has four individual bands (low, low-mid, high-mid, high) if you want to
target a specific frequency range instead of the whole tonal balance.

### 3. Compression
Makes the mix feel more even and controlled. **Glue** is the main knob --
turn it up to make everything feel tighter and more cohesive, like it's
sitting together as one thing rather than separate pieces. Advanced gives
you the classic threshold/ratio/attack/release/makeup controls if you want
to shape exactly how it reacts.

### 4. Saturation
Adds warmth and harmonic character -- subtle analog-style "glue" rather
than distortion. **Drive** is the main knob: more drive means more grit and
richness. Advanced lets you pick the flavor (**Character**: Tape, Tube,
Transistor, or Clip), blend it in with **Mix**, and shape its tone with
**Tone Tilt**.

### 5. Stereo
Controls how wide the mix sounds. **Width** is the main knob for the
overall stereo image. Advanced splits this into three frequency bands --
bass is normally kept narrow (near mono) for a solid, speaker-friendly low
end, while mids and highs can be pushed wider for space and air.

### 6. Limiter
The final stage: sets your overall loudness and makes sure nothing clips.
**Punch vs Loud** is the main knob -- toward "Punch" preserves transients
and dynamics (drums hit harder), toward "Loud" prioritizes squeezing out
maximum loudness. Advanced has the **Ceiling** (max output level),
**Target Loudness** (what loudness it's aiming for, in LUFS), and
**Release** (how fast it recovers after clamping a peak).

---

## Known limitations

AutoMaster is a strong starting point, not a mastering engineer replacement
or a loudness-certification tool. In plain terms:

- **The loudness (LUFS) readout is an estimate**, not an officially
  certified measurement. It's accurate enough to guide decisions but
  shouldn't be quoted on a distributor's loudness-compliance form.
- **The gain-reduction meters (Compression/Limiter tabs) are close
  approximations** for visual feedback, not exact readouts -- trust your
  ears over the exact number.
- **It doesn't detect genre.** It reacts to what it actually measures in
  your audio (dynamics, tone, stereo width), not to "this sounds like
  house music, so...". If your track needs genre-specific treatment, use
  its suggestion as a starting point and adjust from there.
- **Some advanced mastering-suite features aren't in this version yet:**
  comparing against a reference track, a dedicated multiband-dynamics tab,
  and a from-scratch preset browser (your DAW's own project/preset saving
  covers this in the meantime). These are on the list for a future update.

For the full technical rundown (architecture, DSP choices, exact caveats),
see [DEVELOPMENT.md](DEVELOPMENT.md).
