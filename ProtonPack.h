
#ifndef ProtonPack_h
#define ProtonPack_h

#include "Arduino.h"

typedef struct _powercell {    
    const static int num_leds = 8;
    const static int UPDATE_RATE = 60;
    const static int MAX_BRIGHTNESS = 4000;
    int leds[num_leds];
    int offset;
    int last_updated;
    int current_led;
    int current_brightness;
    boolean initializing;

} Powercell;

typedef struct _cyclotron {
    int offset;
    int leds[4];
    int last_updated;
    int current_led;
    int current_brightness;
} Cyclotron;

typedef struct _protonpack { 
    int last_updated;
    int now;
    int started_at;
    boolean is_on;
    boolean is_initializing;
    boolean is_activated;
    Powercell powercell;
    Cyclotron cyclotron;
} Pack;

typedef void (*updatePowercellCallback)(Pack*, Powercell*);
typedef void (*updateCyclotronCallback)(Pack*, Cyclotron*);

class ProtonPack {
public:
    ProtonPack(int power_switch_id, int activate_switch_id, int cyclotron_offset, int powercell_offset);
    void update();
    void initialize();
    void reset();
    void setPowercellUpdateCallback(updatePowercellCallback);
    void setCyclotronUpdateCallback(updateCyclotronCallback);
private:
    Pack _pack;
    Powercell _cell;
    Cyclotron _cyclotron;
    updatePowercellCallback _update_powercell_cb;
    updateCyclotronCallback _update_cyclotron_cb;
    int _cyclotron_offset;
    int _powercell_offset;
    int _power_switch_id;
    int _activate_switch_id;
    void _updatePowercell();
    void _updateCyclotron();
};

#endif
