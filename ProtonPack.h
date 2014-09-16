
#ifndef ProtonPack_h
#define ProtonPack_h

#include <stdint.h>
#include "Arduino.h"

int pp_decrement(int current, int max_value);
int pp_mod_increment(int current, int max_value);

template <typename T,unsigned S>
inline unsigned arraysize(const T (&v)[S]) {
    return S;
}

template<typename Data>
class Vector {
    size_t d_size; // Stores no. of actually stored objects
    size_t d_capacity; // Stores allocated capacity
    Data *d_data; // Stores data
public:
    Vector() : d_size(0), d_capacity(0), d_data(0) {
    }; // Default constructor
    Vector(Vector const &other) : d_size(other.d_size), d_capacity(other.d_capacity), d_data(0) {
        d_data = (Data *) malloc(d_capacity*sizeof(Data));
        memcpy(d_data, other.d_data, d_size*sizeof(Data));
    }; // Copy constuctor
    ~Vector() {
        free(d_data);
    }; // Destructor
    Vector &operator=(Vector const &other) {
        free(d_data);
        d_size = other.d_size;
        d_capacity = other.d_capacity;
        d_data = (Data *) malloc(d_capacity*sizeof(Data));
        memcpy(d_data, other.d_data, d_size*sizeof(Data));
        return *this;
    }; // Needed for memory management
    void push_back(Data const &x) {
        if (d_capacity == d_size) resize();
        d_data[d_size++] = x;
    }; // Adds new value. If needed, allocates more space
    size_t size() const {
        return d_size;
    }; // Size getter
    Data const &operator[](size_t idx) const {
        return d_data[idx];
    }; // Const getter
    Data &operator[](size_t idx) {
        return d_data[idx];
    }; // Changeable getter
private:
    void resize() {
        d_capacity = d_capacity ? d_capacity*2 : 1;
        Data *newdata = (Data *)malloc(d_capacity*sizeof(Data));
        memcpy(newdata, d_data, d_size * sizeof(Data));
        free(d_data);
        d_data = newdata;
    };// Allocates double the old space
};


typedef struct _protonpack {
    int last_updated;
    unsigned long next_call_time;
    unsigned long now;
    unsigned long started_at;
    boolean is_on;
    boolean is_initializing;
    boolean is_firing;
} Pack;

class ProtonPack;

class PackComponent {
public:
    PackComponent(int offset);
    int _offset;
    unsigned long _last_updated;
    unsigned long _started_at;
    unsigned long _next_call_time;
    void callAgainIn(int num_millis);
    bool isReadyToUpdate();
    void setPack(ProtonPack *pack);
    virtual void onUpdate(Pack pack);
    virtual void onActivateButtonPress(Pack pack);
    virtual void onFiringStart(Pack pack);
    virtual void onFiringStop(Pack pack);
    virtual void onPackStartUp(Pack pack);
    virtual void onPackShutDown(Pack pack);
    virtual void onPackInitStart(Pack pack);
    virtual void onPackInitComplete(Pack pack);
    virtual void reset(Pack pack);

protected:
    int offset;
    ProtonPack *_proton_pack;
    void setLed(int ledNumber, int value);

};

class ProtonPack {
public:
    ProtonPack(int power_switch_id,
               int activate_switch_id);
    void update();
    void initialize();
    void reset();
    void addComponent(PackComponent *component);
    void setLed(int offset, int value);

protected:
    Pack _pack;
    Vector<PackComponent*> _components;
    int *_led_state;
    bool _led_state_changed;
    int _power_switch_id;
    int _activate_switch_id;
    int _power_button_state;
    int _activate_button_state;
    int _num_leds;
};

#endif
