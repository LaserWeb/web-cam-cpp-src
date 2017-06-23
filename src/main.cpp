// Copyright 2017 Todd Fleming
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "cam.h"
#include "offset.h"
#include <emscripten.h>
#include <emscripten/bind.h>
#include <stdio.h>

using namespace cam;
using namespace emscripten;

int main()
{
    return 0;
}

val getInterop()
{
    EM_ASM(Module.interop = Module.interop || {};);
    return val::module_property("interop");
}

void print(const PolygonSet& ps)
{
    printf("%d\n", ps.size());
    for (auto& p : ps) {
        printf("    %d\n", p.size());
        for (auto& pt : p)
            printf("        %d, %d\n", x(pt), y(pt));
    }
}

Polygon toPolygon(double scale, const val& v)
{
    val interop = getInterop();
    Polygon result(v["length"].as<unsigned>() / 2);
    static_assert(sizeof(result[0]) == 8, "");

    interop.set("v", v);
    EM_ASM_(
        {
            let v = Module.interop.v;
            let dest = $0 >> 2;
            let len = $1;
            let scale = $2;
            for (let i = 0; i < len; ++i)
                HEAP32[dest++] = v[i] * scale;
        },
        result.data(), result.size() * 2, scale);
    interop.set("v", 0);

    return result;
}

PolygonSet toPolygonSet(double scale, const val& v)
{
    PolygonSet ps(v["length"].as<unsigned>());
    for (size_t i = 0; i < ps.size(); ++i)
        ps[i] = toPolygon(scale, v[i]);
    return ps;
}

val toRawPaths(double scale, const PolygonSet& ps)
{
    val interop = getInterop();
    EM_ASM(Module.interop.result = []);
    for (auto& p : ps) {
        static_assert(sizeof(p[0]) == 8, "");
        EM_ASM_(
            {
                let src = $0 >> 2;
                let len = $1;
                let invScale = 1 / $2;
                let poly = new Float64Array(len);
                for (let i = 0; i < len; ++i)
                    poly[i] = HEAP32[src++] * invScale;
                Module.interop.result.push(poly);
            },
            p.data(), p.size() * 2, scale);
    }
    val result = interop["result"];
    interop.set("result", 0);
    return result;
}

val wrap(PolygonSet ps)
{
    auto destroy = [](PolygonSet* ps) {
        printf("destroy %p\n", ps);
        delete ps;
    };

    auto p = new PolygonSet(std::move(ps));
    printf("wrap %p\n", p);
    val interop = getInterop();
    EM_ASM_(
        {
            let result = Module.interop.result = {};
            result.isPolygonSet = true;
            result.pointer = $0;
            result.destroy = function()
            {
                invoke_vi($1, this.pointer);
                this.pointer = 0;
            };
        },
        p, +destroy);
    val result = interop["result"];
    interop.set("result", 0);
    return result;
}

struct Converter {
    PolygonSet owningPS;
    PolygonSet* ps;

    Converter(double scale, const val& origVal, bool destroy)
    {
        if (origVal["isPolygonSet"].isTrue()) {
            ps = (PolygonSet*)origVal["pointer"].as<unsigned>();
            if (!ps)
                ps = &owningPS;
            else if (destroy) {
                owningPS = std::move(*ps);
                ps = &owningPS;
                origVal.call<void>("destroy");
            }
        } else {
            owningPS = toPolygonSet(scale, origVal);
            ps = &owningPS;
        }
    }

    Converter(const Converter&) = delete;
    Converter(Converter&&) = delete;
    Converter& operator=(const Converter&) = delete;
    Converter& operator=(Converter&&) = delete;

    operator PolygonSet&()
    {
        return *ps;
    }
};

// !!! winding
val clean(double scale, const val& v, bool destroy)
{
    return wrap(cleanPolygonSet(*Converter{ scale, v, destroy }.ps, FlexScan::PositiveWinding{}));
}

val offset(double scale, const val& v, bool destroy, int amount, int arcTolerance, bool closed)
{
    return wrap(FlexScan::offset(*Converter{ scale, v, destroy }.ps, amount, arcTolerance, closed));
}

val getRawPaths(double scale, const val& v, bool destroy)
{
    return toRawPaths(scale, Converter{ scale, v, destroy });
}

EMSCRIPTEN_BINDINGS(my_module)
{
    function("clean", &clean);
    function("offset", &offset);
    function("getRawPaths", &getRawPaths);
}
