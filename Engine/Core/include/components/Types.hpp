#pragma once

#if defined(__cpp_lib_flat_map)
    #include <flat_map>
    namespace vex {
        template<class Key, class T, class Compare = std::less<Key>>
        using vex_map = std::flat_map<Key, T, Compare>;
    }

#else
    #include <map>
    namespace vex {
        template<class Key, class T, class Compare = std::less<Key>>
        using vex_map = std::map<Key, T, Compare>;
    }
#endif
