# Domain Entities — U1: Config & Parameters

## Entity: AngledSlicing::Params

**Type**: Immutable value object  
**Lifetime**: Created at the start of `PrintObject::slice_volumes()`, used throughout slicing of a single object, destroyed after.  
**Thread Safety**: Immutable after construction → inherently thread-safe.

```cpp
namespace Slic3r::AngledSlicing {

struct Params {
    // Factory methods
    static Params from_config(const PrintObjectConfig &config);
    static Params disabled();  // Returns angle=0, direction=0

    // Queries
    bool   enabled()              const;  // angle > epsilon (1e-6)
    double tilt_angle_deg()       const;  // [0, 89]
    double tilt_direction_deg()   const;  // [0, 360)
    double tilt_angle_rad()       const;  // angle_deg * PI / 180
    double tilt_direction_rad()   const;  // direction_deg * PI / 180

private:
    double m_angle_deg     { 0.0 };
    double m_direction_deg { 0.0 };

    Params(double angle, double direction)
        : m_angle_deg(angle), m_direction_deg(direction) {}
};

} // namespace Slic3r::AngledSlicing
```

## Entity: PrintObjectConfig (extended)

Two new fields added to the existing PRINT_CONFIG_CLASS_DEFINE macro:

```cpp
// In PrintObjectConfig, after layer_height:
((ConfigOptionFloat,  angled_slicing_angle))       // degrees, [0, 89], default 0
((ConfigOptionFloat,  angled_slicing_direction))   // degrees, [0, 360), default 0
```

## Entity: PrintConfigDef (extended)

Two new option definitions registered:

```cpp
// In PrintConfig.cpp, init_fff_params():
def = this->add("angled_slicing_angle", coFloat);
def->label = L("Angled slicing angle");
def->category = L("Layers and perimeters");
def->tooltip = L("Angle of the slicing plane relative to horizontal, in degrees. "
                  "0 = standard horizontal layers. Higher values tilt the layer planes "
                  "to improve strength along a specific direction. "
                  "Multiple layers will contact the build plate simultaneously.");
def->sidetext = L("°");
def->min = 0;
def->max = 89;
def->mode = comExpert;
def->set_default_value(new ConfigOptionFloat(0.));

def = this->add("angled_slicing_direction", coFloat);
def->label = L("Angled slicing direction");
def->category = L("Layers and perimeters");
def->tooltip = L("Direction of the slicing plane tilt as a compass bearing, in degrees. "
                  "0 = layers tilt toward +X (right), "
                  "90 = layers tilt toward +Y (back), "
                  "180 = layers tilt toward -X (left), "
                  "270 = layers tilt toward -Y (front).");
def->sidetext = L("°");
def->min = 0;
def->max = 360;
def->mode = comExpert;
def->set_default_value(new ConfigOptionFloat(0.));
```

## Relationships

```
PrintObjectConfig  ──contains──►  angled_slicing_angle (ConfigOptionFloat)
                   ──contains──►  angled_slicing_direction (ConfigOptionFloat)
                   ──read by──►  AngledSlicing::Params::from_config()

AngledSlicing::Params  ──consumed by──►  U2: Transform, ZSchedule, FirstLayer
```
