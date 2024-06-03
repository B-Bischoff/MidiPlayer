#pragma once

#include <iostream>

enum Phase { Attack, Decay, Sustain, Release, Inactive, Retrigger };

struct sEnvelopeADSR
{
	unsigned int keyIndex;

	double attackTime;
	double decayTime;
	double sustainAmplitude;
	double releaseTime;
	double attackAmplitude;
	double triggerOffTime;
	double triggerOnTime;
	bool noteOn;

	bool retrigger;
	double amplitudeBeforeRetrigger;
	double amplitudeAtOffTrigger;

	Phase phase;
	double amplitude;

	sEnvelopeADSR()
	{
		attackTime = .5;
		decayTime = .5,
		attackAmplitude = 1.0;
		sustainAmplitude = 0.8;
		releaseTime = 0.5;
		noteOn = false;
		triggerOffTime = 0.0;
		triggerOnTime = 0.0;
		retrigger = false;
		phase = Inactive;
		amplitude = 0.0;
	}

	double GetAmplitude(double time, bool notePressed)
	{
		if (notePressed && !noteOn)
		{
			triggerOnTime = time;
			noteOn = true;
			//std::cout << "NOTE ON" << std::endl;
		}
		else if (!notePressed && noteOn)
		{
			triggerOffTime = time;
			noteOn = false;
			//std::cout << "NOTE OFF" << std::endl;
		}

		if (triggerOnTime == 0.0f)
			return 0;

		double lifeTime = time - triggerOnTime;

		Phase newPhase = getPhase(time);

		if (newPhase == Retrigger && phase == Release) // Press key while its release phase is not finished
			amplitudeBeforeRetrigger = amplitude;
		else if (newPhase == Release && phase != Release) // Save last amplitude before note off
			amplitudeAtOffTrigger = amplitude;

		phase = newPhase;

		switch (phase)
		{
			case Phase::Attack :
				amplitude = (lifeTime / attackTime) * attackAmplitude;
				break;

			case Phase::Decay :
				retrigger = false;
				lifeTime -= attackTime;
				amplitude = (lifeTime / decayTime) * (sustainAmplitude - attackAmplitude) + attackAmplitude;
				break;

			case Phase::Sustain :
				amplitude = sustainAmplitude;
				break;

			case Phase::Release :
				lifeTime = time - triggerOffTime;
				amplitude = (1.0 - (lifeTime / releaseTime)) * amplitudeAtOffTrigger;
				break;

			case Phase::Inactive :
				amplitude = 0.0;
				triggerOnTime = 0;
				triggerOffTime = 0;
				amplitudeAtOffTrigger = 0;
				break;

			case Phase::Retrigger :
				amplitude = (lifeTime / attackTime) * (attackAmplitude - amplitudeBeforeRetrigger) + amplitudeBeforeRetrigger;
				break;
		}


		if (amplitude <= 0.0001)
			amplitude = 0.0;

		//std::cout << time << " " << amplitude << " " << (retrigger ? 6 : phase) << std::endl;

		return amplitude;
	}

private:
	Phase getPhase(double time)
	{
		if (!noteOn)
		{
			if (time - triggerOffTime <= releaseTime)
				return Phase::Release;
			else
				return Phase::Inactive;
		}

		const double lifeTime = time - triggerOnTime;

		if (triggerOffTime != 0.0 && triggerOnTime != 0.0f && triggerOnTime - triggerOffTime < releaseTime && lifeTime <= attackTime)
			return Phase::Retrigger;

		if (lifeTime <= attackTime)
			return Phase::Attack;
		else if (lifeTime <= attackTime + decayTime)
			return Phase::Decay;
		else
			return Phase::Sustain;
	}
};
