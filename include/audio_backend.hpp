#include "envelope.hpp"

double osc(double hertz, double time, double LFOHertz = 0.0, double LFOAmplitude = 0.0);
double pianoKeyFrequency(int keyId);

struct Base {
	Base() : input(nullptr) {}
	virtual ~Base() {};
	virtual double process() = 0;

	static double time;
	Base* input;
};

struct Master : public Base {
	double process()
	{
		if (!input)
		{
			std::cout << "[WARNING] no input plugged in master" << std::endl;
			return 0;
		}

		double value = input->process();
		std::cout << "master" << std::endl;
		return value;
	}
};

struct KeyboardFrequency : public Base {
	static unsigned int keyIndex;

	double process()
	{
		std::cout << "kb freq" << std::endl;
		return pianoKeyFrequency(keyIndex);
	}
};

struct ADSR : public Base {
	static sEnvelopeADSR* envelope;

	double process()
	{
		double value = input->process() * envelope->GetAmplitude(time);
		std::cout << "adsr" << std::endl;
		return value;
	}
};

struct Oscillator : public Base {
	double process()
	{
		double frequency = input->process();
		double value = osc(frequency, time);
		std::cout << "osc" << std::endl;
		return value;
	}
};
