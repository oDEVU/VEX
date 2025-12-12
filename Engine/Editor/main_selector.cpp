#include "ProjectSelector/ProjectSelector.hpp"
#include "components/GameInfo.hpp"

int main(int argc, char* argv[]) {
    vex::ProjectSelector selector("Vex Project Selector");
    selector.run([]() {});

    return 0;
}
