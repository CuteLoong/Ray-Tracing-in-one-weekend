#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "vec3.h"

class sphere : public hittable
{
private:
    point3 center;
    double radius;
    std::shared_ptr<material> mat_ptr;

public:
    sphere() {}
    sphere(point3 cen, double r, std::shared_ptr<material> m) : center(cen), radius(r), mat_ptr(m){};

    virtual bool hit(const ray &r, double t_min, double t_max, hit_record &rec) const override;
};

bool sphere::hit(const ray &r, double t_min, double t_max, hit_record &rec) const
{
    vec3 oc = r.origin() - center;
    auto a = r.direction().length_squared();
    auto half_b = dot(oc, r.direction());
    auto c = oc.length_squared() - radius * radius;

    auto discriminant = half_b * half_b - a * c;
    if (discriminant < 0)
        return false;
    auto sqrtd = std::sqrt(discriminant);

    // find the nearest root that lies in the acceptable range
    auto root = (-half_b - sqrtd) / a;
    if (root < t_min || root > t_max)
    {
        root = (-half_b + sqrtd) / a;
        if (root < t_min || root > t_max)
        {
            return false;
        }
    }

    rec.t = root;
    rec.p = r.at(rec.t);
    // vec3 outward_normal = (rec.p - center) / radius;
    vec3 outward_normal = unit_vector(rec.p - center);
    rec.set_face_normal(r, outward_normal);
    rec.mat_ptr = mat_ptr;

    return true;
}

#endif