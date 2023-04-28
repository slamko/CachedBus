#ifndef CACHE_BUS_H
#define CACHE_BUS_H

#include "mbed.h"
#include <tuple>
#include <functional>
#include <type_traits>

namespace Cached {
    #define OUT_OF_BOUNDS_ERROR "error: Bus index out of bounds"

    template <class In, class Data>
    class InputRead {
    public:
        virtual Data read(bool inverse_read) = 0;

        operator Data();

        Data read_cached() {
            return data;
        }

        InputRead(In& in);
        InputRead(const InputRead &) = delete;
        InputRead& operator =(const InputRead&) = delete; 

        InputRead(InputRead &&) = default;
        
    protected:
        std::reference_wrapper<In> in;
        Data data;
    };

    class Digital : public InputRead<DigitalIn, int> {
    public:
        using InputRead::InputRead;
        int read(bool inverse_read = false) override;
    };

    class Analog : public InputRead<AnalogIn, float> {
    public:
        using InputRead::InputRead;
        float read(bool inverse_read = false) override;
    };

    template<class T, size_t N>
    class Bus : private NonCopyable<Bus<T, N>> {
    private:
        T list[N];

    public:
        template <class ...PT>
        Bus(PT&& ...list);

        template <size_t I>
        auto get();

        auto operator [](size_t index);

        void read_all(bool inverse_read = false);

        template <size_t I>
        void read(bool inverse_read = false);

        template <size_t I, size_t In, size_t ...Index>
        void read(bool inverse_read = false);

        void read(std::initializer_list<size_t> ids, bool inverse_read = false);
    };

    template <class In, class Data>
    InputRead<In, Data>::InputRead(In &in) : in(in) {}

    template <class In, class Data>
    InputRead<In, Data>::operator Data() {
        return data;
    }

    template <class T, size_t N>
    template <class ...PT>
    Bus<T, N>::Bus(PT&& ...list) : list {list...} {}

    template <class T, size_t N>
    auto Bus<T, N>::operator [](size_t index) {
        if (index >= N) {
            #if __cpp_exceptions 
            //    throw OUT_OF_BOUNDS_ERROR;
            #endif
        }
        
        return this->list[index].read_cached();
    }

    template <class T, size_t N>
    void Bus<T, N>::read_all(bool inverse_read) {
        for (auto &in : list) {
            in.read(inverse_read);
        }
    }

    template <class T, size_t N>
    template <size_t I>
    auto Bus<T, N>::get() {
        static_assert(I < N, OUT_OF_BOUNDS_ERROR);

        return list[I].read_cached();
    }

    template <class T, size_t N>
    template <size_t I>
    void Bus<T, N>::read(bool inverse_read) {
        static_assert(I < N, OUT_OF_BOUNDS_ERROR);
        this->list[I].read(inverse_read);
    }

    template <class T, size_t N>
    template <size_t I, size_t In, size_t ...Index>
    void Bus<T, N>::read(bool inverse_read) {
        static_assert(I < N, OUT_OF_BOUNDS_ERROR);
        this->list[I].read(inverse_read);
        read<In, Index...>();
    }

    template <class T, size_t N>
    void Bus<T, N>::read(std::initializer_list<size_t> ids, bool inverse_read) {
        for (auto id : ids) {
            if (id >= N) {
                #if __cpp_exceptions 
                //    throw OUT_OF_BOUNDS_ERROR;
                #endif
            }

            this->list[id].read(inverse_read);
        }
    }

    template <size_t N>
    using DBus = Bus<Digital, N>;

    template <size_t N>
    using ABus = Bus<Analog, N>;

    template <size_t N>
    using void_if_non_nil = typename std::enable_if_t<N != 0, void>;

    template <size_t N>
    using void_if_nil = typename std::enable_if_t<N == 0, void>;

    template <class T>
    struct assoc_data_type;

    template <>
    struct assoc_data_type<Digital> {
        using type = int;
    };

    template <>
    struct assoc_data_type<Analog> {
        using type = float;
    };

    template <class T>
    using assoc_data_type_t = typename assoc_data_type<std::remove_reference_t<T>>::type;

    template<class ...T>
    class VBus {
    private:
        std::tuple<T...> list;

    public:
        template <class ...PT>
        VBus(PT &&...list);

        VBus(const VBus &) = delete;
        VBus& operator =(const VBus &) = delete;
        
        VBus(VBus &&vbus) = default;
        VBus& operator =(VBus &&vbus) = default;
        
        using data_bus = std::tuple<assoc_data_type_t<T>...>;

        template <size_t ...Ids>
        using bound_data_bus = std::tuple<assoc_data_type_t<decltype(std::get<Ids>(list))>...>;

        template <size_t I>
        auto get();
        
        data_bus read_all(bool inverse_read = false);

        template <bool InverseRead = false, class ...DataArgs>
        void read_all(DataArgs &... dargs);

        template <size_t I>
        auto read(bool inverse_read = false);

        template <size_t I, size_t In, size_t ...Index>
        bound_data_bus<I, In, Index...> read(bool inverse_read = false);

        template <size_t ...Index, class ...DataArgs>
        void read(DataArgs &...dargs);

    private:
        template <size_t I>
        void_if_nil<I - 1u> read_all_iter(data_bus &data, bool inverse_read);

        template <size_t I>
        void_if_non_nil<I - 1u> read_all_iter(data_bus &data, bool inverse_read);

        template <class DBus, size_t I>
        void read_iter(DBus &data, bool inverse_read);

        template <class DBus, size_t I, size_t In, size_t ...Index>
        void read_iter(DBus &data, bool inverse_read);
    };

    template <class ...T>
    template <class ...PT>
    VBus<T...>::VBus(PT &&...list) : list {list...} {}

        template <class ...T>
    template <size_t I>
    auto VBus<T...>::get() {
        return std::get<I>(list).read_cached();
    }

    template <class ...T>
    template <size_t I>
    auto VBus<T...>::read(bool inverse_read) {
        return std::get<I>(list).read(inverse_read);
    }

    template <class ...T>
    template <class DBus, size_t I>
    void VBus<T...>::read_iter(DBus &data, bool inverse_read) {
        std::get<0>(data) = read<I>(inverse_read);
    }

    template <class ...T>
    template <class DBus, size_t I, size_t In, size_t ...Index>
    void VBus<T...>::read_iter(DBus &data, bool inverse_read) {
        auto read_val = std::get<I>(list).read(inverse_read);
        std::get<sizeof...(Index) + 1u>(data) = read_val;
        //auto temp = std::tuple_cat(data, std::make_tuple(read_val));
        read_iter<DBus, In, Index...>(data, inverse_read);
    }

    template <class ...T>
    template <size_t I, size_t In, size_t ...Index>
    auto VBus<T...>::read(bool inverse_read) -> bound_data_bus<I, In, Index...> {
        bound_data_bus<I, In, Index...> dbus;
        read_iter<decltype(dbus), I, In, Index...>(dbus, inverse_read);
        return dbus;
    }

    template <class ...T>
    template <size_t I>
    void_if_nil<I - 1u> VBus<T...>::read_all_iter(data_bus &data, bool inverse_read) {
        auto read_val = read<0>(inverse_read);
        std::get<0>(data) = read_val;
    }
  
  
    template <class ...T>
    template <size_t I> 
    void_if_non_nil<I - 1u> VBus<T...>::read_all_iter(data_bus &data, bool inverse_read) {
        auto read_val = read<I>(inverse_read);
        std::get<I>(data) = read_val;
        read_all_iter<I - 1u>(data, inverse_read);
    }

    template <class ...T>
    auto VBus<T...>::read_all(bool inverse_read) -> data_bus {
        data_bus dbus;
        read_all_iter<std::tuple_size<decltype(list)>::value - 1u>(dbus, inverse_read);
        return dbus;
    }

    template <class ...T>
    template <bool InverseRead, class ...DataArgs>
    void VBus<T...>::read_all(DataArgs &...dargs) {
        std::tie(dargs...) = read_all(InverseRead);
    }

    template <class T>
    struct assoc_type;

    template <>
    struct assoc_type<DigitalIn> {
        using type = Digital;
    };

    template <>
    struct assoc_type<AnalogIn> {
        using type = Analog;
    };

    template <class T>
    using assoc_type_t = typename assoc_type<T>::type;

    template <class ...Args>
    VBus<assoc_type_t<Args>...> make_vbus(Args &&...args) {
        return VBus<assoc_type_t<Args>...>(args...);
    }

    using D = Digital;
    using A = Analog;
}

#endif // CACHE_BUS_H