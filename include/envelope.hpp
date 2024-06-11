#pragma once

#include <iostream>
#include <MidiMath.hpp>

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

	Vec2 controlPoints[8] = {
		{0.0f, 0.0f}, // static 0
		{1.0f, 0.0f}, // ctrl 0
		{1.0f, 1.0f}, // static 1 | attack
		{2.0f, 1.0f}, // ctrl 1
		{2.0f, 0.8f}, // static 2  | decay
		{3.0f, 0.8f}, // static 3 | sustain
		{3.0f, 0.0f}, // ctrl 2
		{4.0f, 0.0f}  // static 4 | release
	};

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
			case Phase::Attack : {
				Vec2 p = bezierQuadratic(controlPoints[0], controlPoints[1], controlPoints[2], \
					inverseLerp(controlPoints[0].x, controlPoints[2].x, lifeTime));
				amplitude = p.y;
				break;
			 }
			case Phase::Decay : {
				retrigger = false;
				Vec2 p = bezierQuadratic(controlPoints[2], controlPoints[3], controlPoints[4], \
					inverseLerp(controlPoints[2].x, controlPoints[4].x, lifeTime));
				amplitude = p.y;
				break;
			}

			case Phase::Sustain : {
				amplitude = controlPoints[4].y;
				break;
			}

			case Phase::Release : {
				Vec2 start = {controlPoints[5].x, (float)amplitudeAtOffTrigger};
				Vec2 p = bezierQuadratic(start, controlPoints[6], controlPoints[7], \
					inverseLerp(0, controlPoints[7].x - controlPoints[5].x, time - triggerOffTime));
				amplitude = p.y;
				break;
			}

			case Phase::Inactive :
				amplitude = 0.0;
				triggerOnTime = 0;
				triggerOffTime = 0;
				amplitudeAtOffTrigger = 0;
				break;

			case Phase::Retrigger :
				amplitude = (lifeTime / controlPoints[2].x) * (controlPoints[2].y - amplitudeBeforeRetrigger) + amplitudeBeforeRetrigger;
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
			if (time - triggerOffTime <= controlPoints[7].x - controlPoints[5].x)
				return Phase::Release;
			else
				return Phase::Inactive;
		}

		const double lifeTime = time - triggerOnTime;

		if (triggerOffTime != 0.0 && triggerOnTime != 0.0f \
				&& triggerOnTime - triggerOffTime < (controlPoints[7].x - controlPoints[5].x) \
				&& lifeTime <= controlPoints[2].x)
			return Phase::Retrigger;

		if (lifeTime <= controlPoints[2].x)
			return Phase::Attack;
		else if (lifeTime <= controlPoints[4].x)
			return Phase::Decay;
		else
			return Phase::Sustain;
	}
};
