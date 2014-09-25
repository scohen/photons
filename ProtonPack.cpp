#include "Arduino.h"
#include <tlc_config.h>
#include <Tlc5940.h>
#include <stdint.h>
#include "ProtonPack.h"


int increment = 50;
int last_debounce_time = 0;
int direction = 1;

int pp_mod_increment(int current, int max_value) {
    int incremented = current += 1;
    if (incremented >= max_value) {
        return 0;
    }
    return incremented;
}

int pp_decrement(int current, int max_value) {
    int decremented = current -= 1;
    if (decremented < 0 ) {
        return max_value;
    }
    return decremented;
}

void PackComponent::callAgainIn(int num_millis) {
    _next_call_time = millis() + (long) num_millis;
}

void PackComponent::setLed(int ledNum, int val) {
    if ( ledNum < _num_leds) {
        _proton_pack->setLed(ledNum + _offset, val);
    }
}


void PackComponent::reset(Pack pack) {
    _last_updated = 0;
    _next_call_time = 0;
}

PackComponent::PackComponent(int offset, int num_leds) {
    _offset = offset;
    _num_leds = num_leds;
}

bool PackComponent::isReadyToUpdate() {
    return _next_call_time <= millis();
}

void PackComponent::onUpdate(Pack pack) {
    Serial.println("onUpdate for PackComponent");
}

void PackComponent::setPack(ProtonPack *pack) {
    _proton_pack = pack;
}

void PackComponent::onActivateButtonPress(Pack pack) {}
void PackComponent::onFiringStart(Pack pack) {}
void PackComponent::onFiringStop(Pack pack) {}
void PackComponent::onPackStartUp(Pack pack) {}
void PackComponent::onPackShutDown(Pack pack) {}
void PackComponent::onPackInitStart(Pack pack) {}
void PackComponent::onPackInitComplete(Pack pack) {}


ProtonPack::ProtonPack(int power_switch_id,
                       int activate_switch_id) {
    _power_switch_id = power_switch_id;
    _activate_switch_id = activate_switch_id;
    _num_leds = NUM_TLCS * 16;
    _led_state = new int[_num_leds];
    _init_millis = 5000;
    reset();
    pinMode(_power_switch_id, INPUT);
    pinMode(_activate_switch_id, INPUT);
}

void resetPack(Pack* _pack) {
    _pack->now = millis();
    _pack->started_at = _pack->now;
    _pack->is_on = false;
    _pack->is_firing = false;
    _pack->is_initializing = false;
}

void ProtonPack::reset() {
    Tlc.clear();
    resetPack(&_pack);
    for(int i=0; i < _components.size(); i++) {
        _components[i]->reset(_pack);
    }

    for(int i=0; i < _num_leds; i++) {
        _led_state[i] = 0;
    }
}

void ProtonPack::initialize() {
    Tlc.init();
    _power_button_state = 0;
    _activate_button_state = 0;
}

void ProtonPack::addComponent(PackComponent *component) {
    _components.push_back(component);
    component->setPack(this);
}

void ProtonPack::setLed(int offset, int value) {
    if (_led_state[offset] != value) {
        _led_state_changed = true;
        _led_state[offset] = value;
    }
}


void ProtonPack::update() {
    _pack.now = millis();
    _led_state_changed = false;

    int latestPowerButtonState = digitalRead(_power_switch_id);;
    int latestActivateButtonState = digitalRead(_activate_switch_id);;

    boolean shutting_down = false;
    boolean starting_up = false;
    boolean is_initializing_now =  (_pack.now - _pack.started_at) < _init_millis;

    if (_pack.is_on) {
        if (is_initializing_now && ! _pack.is_initializing) {
            Serial.println("Init begin");
            for(int i=0; i < _components.size(); i++) {
                PackComponent *c = _components[i];
                c->onPackInitStart(_pack);
            }
        } else if (!is_initializing_now && _pack.is_initializing) {
            for(int i=0; i < _components.size(); i++) {
                PackComponent *c = _components[i];
                c->onPackInitComplete(_pack);
            }
            Serial.println("init end");
        }

       _pack.is_initializing = is_initializing_now;
    }
    
    if (millis() - last_debounce_time > 200) {
        if ( latestPowerButtonState != _power_button_state) {
            if(_pack.is_on) {
                shutting_down = true;
            } else {
                starting_up = true;
            }
            _pack.is_on = !_pack.is_on;
            last_debounce_time = millis();
            _pack.now = last_debounce_time;
            _power_button_state = latestPowerButtonState;
        }

        if (_pack.is_on) {
            if (latestActivateButtonState != _activate_button_state) {
                _pack.is_firing = !_pack.is_firing;
                if (_pack.is_firing) {
                    for(int i=0; i < _components.size(); i++) {
                        PackComponent *c = _components[i];
                        c->onFiringStart(_pack);
                    }
                } else {
                    for(int i=0; i < _components.size(); i++) {
                        PackComponent *c = _components[i];
                        c->onFiringStop(_pack);
                    }
                }
                _activate_button_state = latestActivateButtonState;
                last_debounce_time = millis();
            }
        }
    }

    if (starting_up) {
        Serial.println("starting");
        _pack.now = millis();
        _pack.started_at = millis();
        
        for(int i=0; i < _components.size(); i++) {
            PackComponent *c = _components[i];
            c->reset(_pack);
            c->onPackStartUp(_pack);
        }

    } else if (shutting_down) {
        Serial.println("shutting down");
        for(int i=0; i < _components.size(); i++) {
            PackComponent *c = _components[i];
            c->onPackShutDown(_pack);
        }
        reset();
    }

    if (_pack.is_on) {
        for(int i=0; i < _components.size(); i++) {
            PackComponent *c = _components[i];
            if (c->isReadyToUpdate()) {
                c->onUpdate(_pack);
            }
        }

        for(int i=0; i < _num_leds; i++) {
            Tlc.set(i, _led_state[i]);
        }
    }
    Tlc.update();
}
