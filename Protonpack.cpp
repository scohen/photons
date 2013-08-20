#include "Arduino.h"
#include <tlc_fades.h>
#include <tlc_config.h>
#include <tlc_animations.h>
#include <tlc_progmem_utils.h>
#include <Tlc5940.h>
#include <tlc_servos.h>
#include <tlc_shifts.h>

#include "Protonpack.h"

int increment = 20;

int pp_increment_to_max(int current, int max_value) {
    int incremented = current += 1;
    if (incremented < max_value) {
        return incremented;
    }
    return 0;
}


void _pp_defaultUpdateCyclotron(Pack* pack, Cyclotron* cyclotron) {

    static int BRIGHTNESS_INCREMENT = 80;
    int last_updated = pack->now - cyclotron->last_updated;

    if (last_updated > 1000) {
        cyclotron->current_brightness = 0;
        cyclotron->current_led = pp_increment_to_max(cyclotron->current_led, 4);
        cyclotron->last_updated = pack->now;
    } else if ( last_updated < 500 ) {
        cyclotron->current_brightness += BRIGHTNESS_INCREMENT;
        cyclotron->leds[cyclotron->current_led] = cyclotron->current_brightness;
    } else if (cyclotron->current_brightness > 0) {
        cyclotron->current_brightness -= BRIGHTNESS_INCREMENT;
        cyclotron->leds[cyclotron->current_led] = cyclotron->current_brightness;
    }
}

void _pp_defaultUpdatePowercell(Pack* pack, Powercell* cell) {
    int last_updated = pack->now - cell->last_updated;

    if (cell->initializing) {
        int elapsed_since_start = pack-> now - pack->started_at;
        if (elapsed_since_start > 5000) {
            cell->initializing = false;
        }

        cell->current_brightness += increment;

        if (cell->current_brightness >= 1000 || cell->current_brightness <=0) {
            increment *= -1;
        }

        for(int i=0; i < cell->num_leds; i++) {
            cell->leds[i] = cell->current_brightness;
        }

    } else {
        if (last_updated > cell->UPDATE_RATE) {
            for (int i=0; i<= cell->num_leds; i++) {
                int val = 0;
                if (i < cell->current_led) {
                    val = cell->MAX_BRIGHTNESS;
                }
                cell->leds[i] = val;
            }
            cell->current_led = pp_increment_to_max(cell->current_led, cell->num_leds + 1);
            cell->last_updated = millis();
        }
    }
}

Protonpack::Protonpack() {
    _cell.last_updated = 0;
    _cell.current_led = 0;
    _cell.initializing = true;
    _cell.current_brightness = 0;
    for(int i=0; i < _cell.num_leds; i++) {
        _cell.leds[i] = 0;
    }

    _cyclotron.last_updated = 0;
    _cyclotron.current_led = -1;
    _cyclotron.current_brightness = 0;
    _cyclotron.fade_started = 0;
    _cyclotron.fade_duration = 250;
    for (int i=0; i < 4; i++) {
        _cyclotron.leds[i] = 0;
    }

    _pack.powercell = _cell;
    _pack.cyclotron = _cyclotron;
    _pack.now = millis();
    _pack.started_at = _pack.now;
    setCyclotronUpdateCallback(_pp_defaultUpdateCyclotron);
    setPowercellUpdateCallback(_pp_defaultUpdatePowercell);
}

void Protonpack::initialize() {
    Tlc.init();
}

void Protonpack::_updatePowercell() {
    _update_powercell_cb(&_pack, &_cell);
    for(int i=0; i < _cell.num_leds; i++) {
        Tlc.set(i, _cell.leds[i]);
    }
}

void Protonpack::_updateCyclotron() {
    _update_cyclotron_cb(&_pack, &_cyclotron);
    for(int i=0; i < 4; i++) {
        Tlc.set(i, _cyclotron.leds[i]);
    }
}

void Protonpack::update() {
    _pack.now = millis();
    _updatePowercell();
    //_updateCyclotron();
    Tlc.update();
}

void Protonpack::setPowercellUpdateCallback(updatePowercellCallback cb) {
    _update_powercell_cb = cb;
}

void Protonpack::setCyclotronUpdateCallback(updateCyclotronCallback cb) {
    _update_cyclotron_cb = cb;
}
