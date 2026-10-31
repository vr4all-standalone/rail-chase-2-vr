#include "../native/reticle_size.h"
#include "../native/settings_io.h"
#include <cassert>
#include <cmath>
#include <sstream>
using namespace rchase2;
static bool near(double a,double b){return std::abs(a-b)<1e-5;}
int main(){
    const Settings fresh;
    assert(near(fresh.reticle_min_size,.35)&&near(fresh.reticle_max_size,1));
    assert(near(fresh.reticle_custom_min_size,.35)&&near(fresh.reticle_custom_max_size,1));
    assert(near(reticle_size_reference(1,1.8,reticle_size_for_mode(fresh)),1.8));
    Settings changed=fresh;
    changed.reticle_min_size=.4f;changed.reticle_max_size=1.3f;
    changed.reticle_custom_min_size=.2f;changed.reticle_custom_max_size=.75f;
    changed.reticle_mode=ReticleMode::Ingame;
    assert(near(reticle_size_reference(1,1.8,reticle_size_for_mode(changed)),2.34));
    changed.reticle_mode=ReticleMode::Custom;
    assert(near(reticle_size_reference(1,1.8,reticle_size_for_mode(changed)),1.35));
    assert(near(reticle_size_reference(100,1.8,reticle_size_for_mode(changed)),100*.2*.75));
    std::ostringstream saved;write_settings(saved,changed);std::istringstream input(saved.str());
    const Settings loaded=read_settings(input);
    assert(near(loaded.reticle_min_size,.4)&&near(loaded.reticle_max_size,1.3));
    assert(near(loaded.reticle_custom_min_size,.2)&&near(loaded.reticle_custom_max_size,.75));
    std::istringstream legacy("version=4\ngun_angle_zero_version=1\nreticle_min_size=.55\nreticle_shrink_distance=4.2\n");
    const Settings migrated=read_settings(legacy);
    assert(near(migrated.reticle_min_size,.55)&&near(migrated.reticle_custom_min_size,.55));
    assert(near(migrated.reticle_max_size,1)&&near(migrated.reticle_custom_max_size,1));
    std::istringstream invalid("reticle_max_size=nan\nreticle_custom_max_size=3.1\nreticle_custom_min_size=.01\n");
    const Settings safe=read_settings(invalid);
    assert(near(safe.reticle_max_size,1)&&near(safe.reticle_custom_max_size,1));
    assert(near(safe.reticle_custom_min_size,.35));
}
