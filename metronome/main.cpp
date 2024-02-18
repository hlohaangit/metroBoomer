#include <chrono>
#include <thread>
#include <string>
using namespace std::chrono_literals;

#include <cpprest/http_msg.h>
#include <pigpio.h>

#include "metronome.hpp"
#include "rest.hpp"

// ** Remember to update these numbers to your personal setup. **
#define LED_RED   27
#define LED_GREEN 17
#define BTN_MODE  23
#define BTN_TAP   24

// Mark as volatile to ensure it works multi-threaded.
volatile bool btn_mode_pressed = false;
volatile bool is_startup = true;
size_t blink_freq;
metronome metro;

// Run an additional loop separate from the main one.
void blink() {
	bool on = false;
	
	while (true){
		std::this_thread::sleep_for(std::chrono::milliseconds(blink_freq));


		if (!metro.is_timing() && !is_startup) {
			gpioWrite(LED_GREEN, 1);
			gpioDelay(10000);
			gpioWrite(LED_GREEN, 0);
		}
	}
}

static void check_tap_button(){
	gpioWrite(LED_RED, 1);
	gpioDelay(10000);
	gpioWrite(LED_RED,0);
}

void updateBPM() {
	blink_freq = metro.get_bpm();
	if (blink_freq !=0) {
		blink_freq = (60*1000) / blink_freq;
		is_startup = false;
	}
	else {
		is_startup = true;
	}
}

// -------------------------------end points-------------------------------------
//cors issue
void msg_reply(web::http::http_request msg, size_t content, int mode) {
	web::http::http_response response(200);
	response.headers().add("Access-Control-Allow-Origin", "*");
	if(mode == 1)
		response.set_body(web::json::value::number(content));
	msg.reply(response);
}

// This is called when a GET request is made to "/answer".
void get42(web::http::http_request msg) {
	msg.reply(200, web::json::value::number(42));
}

void getBPM(web::http::http_request msg) {
	//msg.reply(200, web::json::value::number(metro.get_bpm()));
	msg_reply(msg, metro.get_bpm(), 1);
}

void getMIN(web::http::http_request msg) {
	
	//msg.reply(200, web::json::value::number(metro.getMIN_MAX(MIN)));
	msg_reply(msg, metro.getMIN_MAX(0), 1);
}
void getMAX(web::http::http_request msg) {
	
	//msg.reply(200, web::json::value::number(metro.getMIN_MAX(MAX)));
	msg_reply(msg, metro.getMIN_MAX(1), 1);
}

void delMIN(web::http::http_request msg) {
	metro.delMIN_MAX(0);
	updateBPM();
	//msg.reply(200);
	msg_reply(msg, 0 ,0);
}

void delMAX(web::http::http_request msg) {
	metro.delMIN_MAX(1);
	updateBPM();
	//msg.reply(200);
	msg_reply(msg, 0 ,0);
}

void setBPM(web::http::http_request msg){
	pplx::task<utility::string_t>msg_str = msg.extract_string();
	std::string str = utility::conversions::to_utf8string(msg_str.get());
	metro.addNewBPM(std::stoul (str,nullptr,0));
	updateBPM();
	msg_reply(msg, 0 ,0);
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
	if (gpioInitialise() < 0) {
		return 1;
	}

	// ** This is where you would create an instance of your metronome. **
	metronome my_metronome;
	my_metronome.start_timing();

	// Set up the directions of the pins.
	// Be careful here, an input pin set as an output could burn out.
	gpioSetMode(LED_RED, PI_OUTPUT);
	gpioSetMode(LED_GREEN, PI_OUTPUT);
	gpioSetMode(BTN_MODE, PI_INPUT);
	gpioSetMode(BTN_TAP, PI_INPUT);
	// Note that you can also set a pull-down here for the button,
	// if you do not want to use the physical resistor.
	//pullUpDnControl(BTN_MODE, PUD_DOWN);

	// Configure the rest services after setting up the pins,
	// but before we start using them.
	// ** You should replace these with the BPM endpoints. **
	auto get42_rest = rest::make_endpoint("/answer");
	auto getBPM_rest = rest::make_endpoint("/bpm");
	auto getMIN_rest = rest::make_endpoint("/bpm/min");
	auto getMAX_rest = rest::make_endpoint("/bpm/max");

	getBPM_rest.support(web::http::methods::GET, getBPM);
	getBPM_rest.support(web::http::methods::PUT, setBPM);
	getMIN_rest.support(web::http::methods::GET, getMIN);
	getMIN_rest.support(web::http::methods::DEL, delMIN);
	getMAX_rest.support(web::http::methods::GET, getMAX);
	getMAX_rest.support(web::http::methods::DEL, delMAX);
	get42_rest.support(web::http::methods::GET, get42);
	// Start the endpoints in sequence.

	getBPM_rest.open().wait();
	getMIN_rest.open().wait();
	getMAX_rest.open().wait();
	get42_rest.open().wait();


	// Use a separate thread for the blinking.
	// This way we do not have to worry about any delays
	// caused by polling the button state / etc.
	std::thread blink_thread(blink);
	blink_thread.detach();

	blink_freq = 10000;
	
	while(true) {
		//learning mode
		if (gpioRead(BTN_MODE)==1){
			printf("Learning button pressed\n");
			if(!metro.is_timing())
				metro.start_timing();
			else {
				metro.stop_timing();
				updateBPM();
			}
			//btn_mode_pressed = !btn_mode_pressed;

			gpioDelay(200000);
		}
		if (metro.is_timing()) {
			if (gpioRead(BTN_TAP)==1) {
				printf("Tap button pressed!\n");
				metro.tap();
				check_tap_button();
				gpioDelay(200000);
			}
		}

		std::this_thread::sleep_for(10ms);
	}

	gpioTerminate();

	return 0;
}
