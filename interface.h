// Author: Rafael de Bie
// Copyright (C) 2023-2023 Rafael de Bie

// Created for use with arduino

#ifndef interface_h
#define interface_h

typedef float		coef_t;
typedef int			inpt_t;

// The PWMSignal class is used to communicate numerical values at a glance.
// The PWMSignal runs in 2 modes.
// In the 'Buffered' mode the PWMSignal holds an internal int, and is updated when called.
// In the 'External' mode, the PWMSignal reads an int*, and is updated on refresh.
class PWMSignal
{
	public:
		PWMSignal(int pin); 		// Creates a PWMSignal and hooks it to pin 'pin'
		void refresh();			// Maybe the buffered value is modified externally?

		// Buffered mode
		void setValue(inpt_t value);	// Sets the value of the PWMSignal to that position
		void switchToBuffered();

		// External mode
		void setReference(inpt_t *ref);			// Tell the PWMSignal where that value is
		void switchToExternal(inpt_t *ref);		// Switch to external mode

		// Display / Calibration
		void setCoefficients(coef_t a, coef_t b);	// Sets the coefficients a and b

		void switchToLinear();				// Switches to linear equation
		void switchToLinear(coef_t a, coef_t b);	// Sets the coefficients: ax+b

		void switchToLogarithmic();			// Switches to logarithmic equation
		void switchToLogarithmic(coef_t a, coef_t b);	// Sets the coefficients: aln(bx)

		void switchToExponential();			// Switches to exponential equation
		void switchToExponential(coef_t a, coef_t b);	// Sets the coefficients: ae^(bx)

		// If custom function is needed, switch to this.
		// (int)(*)(int, int, int) ->
		// return value is the pwm signal
		// (int, int, int) -> (input, a, b)
		void switchToCustom( int (*)(inpt_t, coef_t, coef_t) );

#ifdef DEBUG
		coef_t coefficient_a;
		coef_t coefficient_b;
		inpt_t *current;
		inpt_t buffered;
		inpt_t rawInput;
		int applyEquation(inpt_t value);
#endif
	private:
#ifndef DEBUG
		inpt_t *current;
		inpt_t buffered;
		inpt_t rawInput;
#endif

		short mode; // 0: linear, 1: logarithmic, 2: exponential, 3: custom
		int (*customEquation)(inpt_t, coef_t, coef_t);

#ifndef DEBUG
		coef_t coefficient_a;
		coef_t coefficient_b;
#endif

		int pin;

		inline int calculateLinear(inpt_t x);
		inline int calculateLogarithmic(inpt_t x);
		inline int calculateExponential(inpt_t x);
#ifndef DEBUG
		int applyEquation(inpt_t value);
#endif
};

using Dial=PWMSignal;

#endif
