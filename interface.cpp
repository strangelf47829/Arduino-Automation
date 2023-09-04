#include "interface.h"
#include "Arduino.h"
#include "math.h"

#define LOGFUNC(x)	log(x)
#define POWFUNC(n,x)	pow(n,x)
#define ROUNDFUNC(x)	round(x)


PWMSignal::PWMSignal(int pin)
{
	// Set pin
	this->pin = pin;

	// Set default values
	buffered = 0;
	rawInput = 0;
	current = &buffered;

	// Start in linear mode: ax+b | a = 1, b = 0
	mode = 0;
	this->coefficient_a = 1;
	this->coefficient_b = 0;

	// Set the pin to 'OUTPUT'
	pinMode(pin, OUTPUT);
}

inline int PWMSignal::calculateLinear(inpt_t x)
{ 	
#ifdef DEBUG
	Serial.print("Equation is ");
	Serial.print(this->coefficient_a);
	Serial.print('(');
	Serial.print(x);
	Serial.print(")+");
	Serial.println(this->coefficient_b);
#endif
	return ROUNDFUNC(this->coefficient_a * x + this->coefficient_b); }

inline int PWMSignal::calculateLogarithmic(inpt_t x)
{ 	
#ifdef DEBUG
	Serial.print("Equation is ");
	Serial.print(this->coefficient_a);
	Serial.print('ln(');
	Serial.print(x);
	Serial.print(this->coefficient_b);
	Serial.println(')');
#endif
	return ROUNDFUNC(this->coefficient_a * LOGFUNC(x * this->coefficient_b)); }

inline int PWMSignal::calculateExponential(inpt_t x)
{ 	
#ifdef DEBUG
	Serial.print("Equation is ");
	Serial.print(this->coefficient_a);
	Serial.print('e^(');
	Serial.print(x);
	Serial.print(this->coefficient_b);
	Serial.println(")");
#endif
	return ROUNDFUNC(this->coefficient_a * POWFUNC(EULER, x * this->coefficient_b)); }

void PWMSignal::setCoefficients(coef_t a, coef_t b)
{ this->coefficient_a = a; this->coefficient_b = b; }

void PWMSignal::refresh()
{ if(current == &buffered) { analogWrite(this->pin, *current); } else { analogWrite(this->pin, applyEquation(*current)); }}

void PWMSignal::switchToBuffered()
{ this->current = &this->buffered; }

void PWMSignal::switchToExternal(inpt_t *addr)
{ this->current = addr; }

void PWMSignal::setReference(inpt_t *addr)
{ this->current = addr; refresh(); }

int PWMSignal::applyEquation(inpt_t value)
{
	switch(this->mode)
	{
		case 0:	// Linear
			return calculateLinear(value);
		case 1:	// Logarithmic
			return calculateLogarithmic(value);
		case 2:	// Exponential
			return calculateExponential(value);
		case 3:	// Custom
			return this->customEquation(value, coefficient_a, coefficient_b);
		default:// Something went wrong. Default to raw input
			return value;
	}
}

void PWMSignal::setValue(inpt_t value)
{
	// Store 'input' to the raw input
	this->rawInput = value;

	// Calculate value based on the selected equation
	this->buffered = applyEquation(value);

	// Then update the buffered value, but only if 'current' is also the address of 'buffered'
	if(current == &buffered)
		refresh();
}

void PWMSignal::switchToLinear()
{ this->mode = 0; }

void PWMSignal::switchToLogarithmic()
{ this->mode = 1; }

void PWMSignal::switchToExponential()
{ this->mode = 2; }

void PWMSignal::switchToCustom( int (*equation) (inpt_t, coef_t, coef_t) )
{ this->mode = 3; this->customEquation = equation; }

void PWMSignal::switchToLinear(coef_t a, coef_t b)
{ this->mode = 0; setCoefficients(a, b); }

void PWMSignal::switchToLogarithmic(coef_t a, coef_t b)
{ this->mode = 1; setCoefficients(a, b); }

void PWMSignal::switchToExponential(coef_t a, coef_t b)
{ this->mode = 2; setCoefficients(a, b); }
