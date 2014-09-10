#include "Arduino.h"
//#include <tlc_fades.h>
#include <tlc_config.h>
//#include <tlc_animations.h>
//#include <tlc_progmem_utils.h>
#include <Tlc5940.h>
//#include <tlc_servos.h>
//#include <tlc_shifts.h>
#include <typeinfo>
#include "ProtonPack.h"

int increment = 50;
int last_debounce_time = 0;
int direction = 1;

int pp_increment_to_max(int current, int max_value) {
    int incremented = current += 1;
    if (incremented >= max_value) {
        return 0;
    }
    return incremented;
}

int pp_decrement(int current, int max_value) {
    int incremented = current -= 1;
    if (incremented < 0 ) {
        return max_value;
    }
    return incremented;
}

void PackComponent::callAgainIn(int num_millis) {
    _next_call_time = millis() + (long) num_millis;
}

void PackComponent::setLed(int ledNum, int val) {
    Tlc.set(ledNum + _offset, val);
}


void PackComponent::reset(Pack* pack) {
    _last_updated = 0;
    _next_call_time = 0;
}

PackComponent::PackComponent(int offset) {
    _offset = offset;
}

bool PackComponent::isReadyToUpdate() {
    return _next_call_time <= millis();
}

void PackComponent::onUpdate(Pack* pack) {
    Serial.println("onUpdate for PackComponent");
}
void PackComponent::onActivateButtonPress(Pack* pack){}
void PackComponent::onFiringStart(Pack* pack){}
void PackComponent::onFiringStop(Pack* pack){}
void PackComponent::onPackStartUp(Pack* pack){}
void PackComponent::onPackShutDown(Pack* pack){}
void PackComponent::onPackInitStart(Pack* pack){}
void PackComponent::onPackInitComplete(Pack* pack){}


ProtonPack::ProtonPack(int power_switch_id,
                       int activate_switch_id) {
    _power_switch_id = power_switch_id;
    _activate_switch_id = activate_switch_id;
    reset();
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
    for(int i=0; i < _components.size(); i++) {
        _components[i]->reset(&_pack);
    }
}

void ProtonPack::initialize() {
    Tlc.init();
    pinMode(_power_switch_id, INPUT);


    _power_button_state = 0;
    _activate_button_state = 0;
}

void ProtonPack::addComponent(PackComponent *component) {
    _components.push_back(component);
}

void ProtonPack::update() {
    _pack.now = millis();

    int latestPowerButtonState = digitalRead(_power_switch_id);;
    int latestActivateButtonState = digitalRead(_activate_switch_id);;

    boolean shutting_down = false;
    boolean starting_up = false;
    boolean is_initializing_now = (_pack.now - _pack.started_at) < 5000;

    if (is_initializing_now && ! _pack.is_initializing) {
        for(int i=0; i < _components.size(); i++) {
            PackComponent *c = _components[i];
            Serial.println("Pack init start");
            c->onPackInitStart(&_pack);
        }
    } else if (!is_initializing_now && _pack.is_initializing) {
        for(int i=0; i < _components.size(); i++) {
            PackComponent *c = _components[i];
            Serial.println("Pack init stop");
            c->onPackInitComplete(&_pack);
        }
    }

    _pack.is_initializing = is_initializing_now;

    if (millis() - last_debounce_time > 300) {
        if ( latestPowerButtonState != _power_button_state) {
            Serial.println("Switch detected");
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
                _activate_button_state = latestActivateButtonState;
                last_debounce_time = millis();
            }
        }
    }

    if (starting_up) {
        _pack.started_at = _pack.now;
        for(int i=0; i < _components.size(); i++) {
            Serial.println("Calling onPackStartUp");
            PackComponent *c = _components[i];
            c->onPackStartUp(&_pack);
        }

    } else if (shutting_down) {
        for(int i=0; i < _components.size(); i++) {
            Serial.println("Calling onPackShutDown");
            PackComponent *c = _components[i];
            c->onPackShutDown(&_pack);
        }
        Tlc.clear();
    }

    if (_pack.is_on) {
        for(int i=0; i < _components.size(); i++) {
            PackComponent *c = _components[i];
            if (c->isReadyToUpdate()) {
                c->onUpdate(&_pack);
            }
        }
    }
    Tlc.update();
}