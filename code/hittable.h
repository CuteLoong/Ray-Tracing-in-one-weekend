#ifndef HITTABLE_H
#define HITTABLE_H

#include "rtweekend.h"

class material;

struct hit_record
{
    point3 p;
    vec3 normal;
    std::shared_ptr<material> mat_ptr;
    double t;
    bool front_face;

    inline void set_face_normal(const ray &r, const vec3 &outword_normal)
    {
        front_face = dot(r.direction(), outword_normal) < 0;
        normal = front_face ? outword_normal : -outword_normal;
    }
};

class hittable
{
private:
public:
    virtual bool hit(const ray &r, double t_min, double t_max, hit_record &rec) const = 0;
};

#endif