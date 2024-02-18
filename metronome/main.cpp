#include <chrono>
#include <thread>
#include <iostream>
using namespace std::chrono_literals;

#include <cpprest/http_msg.h>
//#include <wiringPi.h>
//#include <pigpio.h>

#include "metronome.hpp"
#include "rest.hpp"

// ** Remember to update these numbers to your personal setup. **
#define LED_RED   27
#define LED_GREEN 17
#define BTN_MODE  23
#define BTN_TAP   24

// Mark as volatile to ensure it works multi-threaded.
volatile bool btn_mode_pressed = false;

//temporary variables for testing the api
int bpm = 0;
int min_bpm = 0;
int max_bpm = 0;

/*

// Run an additional loop separate from the main one.
void blink() {
	bool on = false;
	// ** This loop manages LED blinking. **
	while (true) {
		// The LED state will toggle once every second.
		std::this_thread::sleep_for(1s);

		// Perform the blink if we are pressed,
		// otherwise just set it to off.
		if (btn_mode_pressed)
			on = !on;
		else
			on = false;

		// HIGH/LOW probably equal 1/0, but be explicit anyways.
		// digitalWrite(LED_RED, (on) ? HIGH : LOW);
		gpioWrite(LED_RED, (on) ? 1 : 0);

	}
}

*/

// request handlers
void get_bpm(web::http::http_request req) {
	req.reply(200, web::json::value::number(bpm));
}

void put_bpm(web::http::http_request req) {
	req.extract_json().then([&](web::json::value req_json){
		int val = req_json[U("bpm")].as_integer();
		bpm = val;

		if(min_bpm == 0 || bpm < min_bpm){
			min_bpm = bpm;
		}

		if(max_bpm == 0 || bpm > max_bpm){
			max_bpm = bpm;
		}
	}).wait();

	req.reply(200); 
}

void get_min_bpm(web::http::http_request req){
	req.reply(200, web::json::value::number(min_bpm));
}

void delete_min_bpm(web::http::http_request req){
	min_bpm = 0;
	req.reply(200);
}

void get_max_bpm(web::http::http_request req){
	req.reply(200, web::json::value::number(max_bpm));
}

void delete_max_bpm(web::http::http_request req){
	max_bpm = 0;
	req.reply(200);
}

// This program shows an example of input/output with GPIO, along with
// a dummy REST service.
// ** You will have to replace this with your metronome logic, but the
//    structure of this program is very relevant to what you need. **
int main() {
	// WiringPi must be initialized at the very start.
	// This setup method uses the Broadcom pin numbers. These are the
	// larger numbers like 17, 24, etc, not the 0-16 virtual ones.
	// wiringPiSetupGpio();
/*
	if (gpioInitialise() < 0) {
		return 1;
	}
*/

	// Set up the directions of the pins.
	// Be careful here, an input pin set as an output could burn out.
	// pinMode(LED_RED, OUTPUT);
/*
	gpioSetMode(LED_RED, PI_OUTPUT);
	gpioSetMode(LED_GREEN, PI_OUTPUT);
	// pinMode(BTN_MODE, INPUT);
	gpioSetMode(BTN_MODE, PI_INPUT);
	gpioSetMode(BTN_TAP, PI_INPUT);
	// Note that you can also set a pull-down here for the button,
	// if you do not want to use the physical resistor.
	//pullUpDnControl(BTN_MODE, PUD_DOWN);
*/

	// Configure the rest services after setting up the pins,
	// but before we start using them.
	// ** You should replace these with the BPM endpoints. **
	auto bpm_rest = rest::make_endpoint("/bpm");
	bpm_rest.support(web::http::methods::GET, get_bpm);
	bpm_rest.support(web::http::methods::PUT, put_bpm);

	auto min_bpm_rest = rest::make_endpoint("/bpm/min");
	min_bpm_rest.support(web::http::methods::GET, get_min_bpm);
	min_bpm_rest.support(web::http::methods::DEL, delete_min_bpm);

	auto max_bpm_rest = rest::make_endpoint("/bpm/max");
	max_bpm_rest.support(web::http::methods::GET, get_max_bpm);
	max_bpm_rest.support(web::http::methods::DEL, delete_max_bpm);

	// Start the endpoints in sequence.
	bpm_rest.open().wait();
	min_bpm_rest.open().wait();
	max_bpm_rest.open().wait();

	while(true);

	// Use a separate thread for the blinking.
	// This way we do not have to worry about any delays
	// caused by polling the button state / etc.
/*
	std::thread blink_thread(blink);
	blink_thread.detach();
*/
/*
	// ** This loop manages reading button state. **
	while (true) {
		// We are using a pull-down, so pressed = HIGH.
		// If you used a pull-up, compare with LOW instead.
		// btn_mode_pressed = (digitalRead(BTN_MODE) == HIGH);
		btn_mode_pressed = (gpioRead(BTN_MODE) == 1);
		// Delay a little bit so we do not poll too heavily.
		std::this_thread::sleep_for(10ms);

		// ** Note that the above is for testing when the button
		// is held down. For detecting "taps", or momentary
		// pushes, you need to check for a "rising edge" -
		// this is when the button changes from LOW to HIGH... **
	}
*/
	return 0;
}
