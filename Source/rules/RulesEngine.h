#pragma once
#include "ChainParameters.h"
#include "../analysis/AnalysisTypes.h"

// AnalysisResult -> ChainParameters. This is deliberately a tiny virtual
// interface: DefaultRulesEngine below is a hand-written heuristic mapping,
// but the processor only ever talks to IRulesEngine, so a trained model
// (e.g. a small regression net over the same AnalysisResult feature vector)
// can be dropped in later behind this same seam with no other code changes.
class IRulesEngine
{
public:
    virtual ~IRulesEngine() = default;
    virtual ChainParameters derive(const AnalysisResult& analysis) const = 0;
};

class DefaultRulesEngine : public IRulesEngine
{
public:
    ChainParameters derive(const AnalysisResult& analysis) const override;
};
