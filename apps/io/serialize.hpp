#pragma once

#include <assert.h>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>

namespace __serialize_detail {
  using stream = std::vector<uint8_t>;
  using stream_ptr = stream::iterator;
  using stream_cptr = stream::const_iterator;
  template <size_t> struct __pos { };
}

template <class T> size_t get_size(const T& obj);

namespace __serialize_detail {
  template <class T> struct get_size_helper;

  template <class T>
  struct get_size_helper<std::vector<T>> {
    static size_t value(const std::vector<T>& obj) {
      return std::accumulate(obj.begin(), obj.end(), sizeof(size_t),
        [](size_t acc, const T& cur) { return acc + get_size(cur); });
    }
  };

  template <>
  struct get_size_helper<std::string> {
    static size_t value(const std::string& obj) {
      return sizeof(size_t) + obj.length() * sizeof(uint8_t);
    }
  };

  template <class tuple_type>
  size_t get_tuple_size(const tuple_type& obj, __pos<0>) {
    constexpr size_t idx = std::tuple_size<tuple_type>::value - 1;
    return get_size(std::get<idx>(obj));
  }

  template <class tuple_type, size_t pos>
  size_t get_tuple_size(const tuple_type& obj, __pos<pos>) {
    constexpr size_t idx = std::tuple_size<tuple_type>::value - pos - 1;
    size_t acc = get_size(std::get<idx>(obj));
    return acc + get_tuple_size(obj, __pos<pos - 1>());
  }

  template <class ...T>
  struct get_size_helper<std::tuple<T...>> {
    static size_t value(const std::tuple<T...>& obj) {
      return get_tuple_size(obj, __pos<sizeof...(T) - 1>());
    }
  };

  template <class T>
  struct get_size_helper {
    static size_t value(const T& obj) { return sizeof(T); }
  };
}

template <class T>
size_t get_size(const T& obj) {
  return __serialize_detail::get_size_helper<T>::value(obj);
}

namespace __serialize_detail {
  template <class T> struct serialize_helper;

  template <class T> void serializer(const T& obj, stream_ptr&);

  template <class tuple_type>
  void serialize_tuple(const tuple_type& obj, stream_ptr& res, __pos<0>) {
    constexpr size_t idx = std::tuple_size<tuple_type>::value - 1;
    serializer(std::get<idx>(obj), res);
  }

  template <class tuple_type, size_t pos>
  void serialize_tuple(const tuple_type& obj, stream_ptr& res, __pos<pos>) {
    constexpr size_t idx = std::tuple_size<tuple_type>::value - pos - 1;
    serializer(std::get<idx>(obj), res);
    serialize_tuple(obj, res, __pos<pos - 1>());
  }

  template <class... T>
  struct serialize_helper<std::tuple<T...>> {
    static void apply(const std::tuple<T...>& obj, stream_ptr& res) {
      __serialize_detail::serialize_tuple(obj, res,
        __serialize_detail::__pos<sizeof...(T) - 1>());
    }
  };

  template <>
  struct serialize_helper<std::string> {
    static void apply(const std::string& obj, stream_ptr& res) {
      serializer(obj.length(), res);
      for (const auto& cur : obj) serializer(cur, res);
    }
  };

  template <class T>
  struct serialize_helper<std::vector<T>> {
    static void apply(const std::vector<T>& obj, stream_ptr& res) {
      serializer(obj.size(), res);
      for (const auto& cur : obj) serializer(cur, res);
    }
  };

  template <class T>
  struct serialize_helper {
    static void apply(const T& obj, stream_ptr& res) {
      const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&obj);
      copy(ptr, ptr + sizeof(T), res);
      res += sizeof(T);
    }
  };

  template <class T>
  void serializer(const T& obj, stream_ptr& res) {
    serialize_helper<T>::apply(obj, res);
  }
}

template <class T>
void serialize(const T& obj, __serialize_detail::stream& res) {
  size_t offset = res.size();
  size_t size = get_size(obj);
  res.resize(res.size() + size);
  __serialize_detail::stream_ptr it = res.begin() + offset;
  __serialize_detail::serializer(obj, it);
  assert(res.begin() + offset + size == it);
}

namespace __serialize_detail {
  template <class T> struct deserialize_helper;

  template <class T>
  struct deserialize_helper {
    static T apply(stream_cptr& begin, stream_cptr end) {
      assert(begin + sizeof(T) <= end);
      T val;
      uint8_t* ptr = reinterpret_cast<uint8_t*>(&val);
      copy(begin, begin + sizeof(T), ptr);
      begin += sizeof(T);
      return val;
    }
  };

  template <class T>
  struct deserialize_helper<std::vector<T>> {
    static std::vector<T> apply(stream_cptr& begin, stream_cptr end) {
      size_t size = deserialize_helper<size_t>::apply(begin, end);
      std::vector<T> vect(size);
      for (size_t i = 0; i < size; ++i)
        vect[i] = std::move(deserialize_helper<T>::apply(begin, end));
      return vect;
    }
  };

  template <>
  struct deserialize_helper<std::string> {
    static std::string apply(stream_cptr& begin, stream_cptr end) {
      size_t size = deserialize_helper<size_t>::apply(begin, end);
      if (size == 0U) return std::string();
      std::string str(size, '\0');
      for (size_t i = 0; i < size; ++i) {
        str.at(i) = deserialize_helper<uint8_t>::apply(begin, end);
      }
      return str;
    }
  };

  template <class tuple_type>
  void deserialize_tuple(tuple_type& obj,
      stream_cptr& begin, stream_cptr end, __pos<0>) {
    constexpr size_t idx = std::tuple_size<tuple_type>::value - 1;
    typedef typename std::tuple_element<idx, tuple_type>::type T;
    std::get<idx>(obj) = std::move(deserialize_helper<T>::apply(begin, end));
  }

  template <class tuple_type, size_t pos>
  void deserialize_tuple(tuple_type& obj,
      stream_cptr& begin, stream_cptr end, __pos<pos>) {
    constexpr size_t idx = std::tuple_size<tuple_type>::value - pos - 1;
    typedef typename std::tuple_element<idx, tuple_type>::type T;
    std::get<idx>(obj) = std::move(deserialize_helper<T>::apply(begin, end));
    deserialize_tuple(obj, begin, end, __pos<pos - 1>());
  }

  template <class... T>
  struct deserialize_helper<std::tuple<T...>> {
    static std::tuple<T...> apply(stream_cptr& begin, stream_cptr end) {
      std::tuple<T...> ret;
      deserialize_tuple(ret, begin, end, __pos<sizeof...(T) - 1>());
      return ret;
    }
  };
}

template <class T>
T deserialize(
    __serialize_detail::stream_cptr& begin,
    __serialize_detail::stream_cptr end) {
  return __serialize_detail::deserialize_helper<T>::apply(begin, end);
}

template <class T>
T deserialize(const __serialize_detail::stream& res) {
  __serialize_detail::stream_cptr it = res.begin();
  return deserialize<T>(it, res.end());
}
