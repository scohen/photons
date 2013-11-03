
#ifndef ProtonPack_h
#define ProtonPack_h

#include "Arduino.h"

typedef struct _powercell {    
    const static int num_leds = 14;
    const static int UPDATE_RATE = 30;
    const static int MAX_BRIGHTNESS = 2000;
    int leds[num_leds];
    int offset;
    unsigned long last_updated;
    int current_led;
    int current_brightness;
    boolean initializing;

} Powercell;

typedef struct _cyclotron {
    int offset;
    int leds[4];
    unsigned long last_updated;
    int current_led;
    int current_brightness;
} Cyclotron;

typedef struct _graph {
    const static int num_leds = 15;
    unsigned long last_updated;    
    int leds[num_leds];
    int iteration;
}Graph;

typedef struct _protonpack { 
    int last_updated;
    unsigned long now;
    unsigned long started_at;    
    boolean is_on;
    boolean is_initializing;    
    boolean is_firing;
    Powercell powercell;
    Cyclotron cyclotron;
    Graph graph;
} Pack;

typedef void (*updatePowercellCallback)(Pack*, Powercell*);
typedef void (*updateCyclotronCallback)(Pack*, Cyclotron*);
typedef void (*updateGraphCallback)(Pack*, Graph*);

class ProtonPack {
public:
    ProtonPack(int power_switch_id,
               int activate_switch_id,
               int cyclotron_offset,
               int powercell_offset,
               int graph_offset);
    void update();
    void initialize();
    void reset();
    void setPowercellUpdateCallback(updatePowercellCallback);
    void setCyclotronUpdateCallback(updateCyclotronCallback);
    void setGraphUpdateCallback(updateGraphCallback);
    
private:
    Pack _pack;
    Powercell _cell;
    Cyclotron _cyclotron;
    Graph _graph;
    updatePowercellCallback _update_powercell_cb;
    updateCyclotronCallback _update_cyclotron_cb;
    updateGraphCallback _update_graph_cb;
    int _cyclotron_offset;
    int _powercell_offset;
    int _power_switch_id;
    int _activate_switch_id;
    int _graph_offset;
    int _power_button_state;
    int _activate_button_state;
    void _updatePowercell();
    void _updateCyclotron();
    void _updateGraph();
};

#endif
