// Ported from CAPS Reverb.
// This effect is GPL code.

#pragma once

#include <Reverb.h>

#include <QMap>

#include "effects/backends/effectprocessor.h"
#include "util/class.h"
#include "util/types.h"

class ReverbGroupState : public EffectState {
  public:
    ReverbGroupState(const mixxx::EngineParameters& engineParameters)
            : EffectState(engineParameters),
              sampleRate(engineParameters.sampleRate()),
              sendPrevious(0) {
    }
    ~ReverbGroupState() override = default;

    void engineParametersChanged(const mixxx::EngineParameters& engineParameters) {
        sampleRate = engineParameters.sampleRate();
        sendPrevious = 0;
    }

    float sampleRate;
    float sendPrevious;
    MixxxPlateX2 reverb;
};

class ReverbEffect : public EffectProcessorImpl<ReverbGroupState> {
  public:
    ReverbEffect() = default;
    ~ReverbEffect() override = default;

    static QString getId();
    static EffectManifestPointer getManifest();
    bool isReadyForDisable() override {
        return m_isReadyForDisable;
    };

    void loadEngineEffectParameters(
            const QMap<QString, EngineEffectParameterPointer>& parameters) override;

    void processChannel(
            ReverbGroupState* pState,
            const CSAMPLE* pInput,
            CSAMPLE* pOutput,
            const mixxx::EngineParameters& engineParameters,
            const EffectEnableState enableState,
            const GroupFeatureState& groupFeatures) override;

  private:
    QString debugString() const {
        return getId();
    }

    bool m_isReadyForDisable = false;

    EngineEffectParameterPointer m_pDecayParameter;
    EngineEffectParameterPointer m_pBandWidthParameter;
    EngineEffectParameterPointer m_pDampingParameter;
    EngineEffectParameterPointer m_pSendParameter;

    DISALLOW_COPY_AND_ASSIGN(ReverbEffect);
};

float averageSampleDifferenceEnergy(const SINT samplesPerBuffer,
        const CSAMPLE* buffer_in,
        const CSAMPLE* buffer_out,
        const SINT tailCheckLength);
