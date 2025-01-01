// Copyright 2024-2025 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//
#include "upa/url.h"

#include <emscripten/bind.h>

using namespace emscripten;

class URL {
public:
    URL(const std::string& url_str) {
        m_url.parse(url_str, nullptr);
        m_base_valid = true;
    }

    URL(const std::string& url_str, const std::string& base_str) {
        upa::url base;
        base.parse(base_str, nullptr);
        m_url.parse(url_str, &base);
        m_base_valid = base.is_valid();
    }

    std::string get_href() const {
        return std::string{ m_url.href() };
    }
    void set_href(const std::string& val) {
        m_url.href(val);
    }

    std::string get_origin() const {
        return m_url.origin();
    }

    std::string get_protocol() const {
        return std::string{ m_url.protocol() };
    }
    void set_protocol(const std::string& val) {
        m_url.protocol(val);
    }

    std::string get_username() const {
        return std::string{ m_url.username() };
    }
    void set_username(const std::string& val) {
        m_url.username(val);
    }

    std::string get_password() const {
        return std::string{ m_url.password() };
    }
    void set_password(const std::string& val) {
        m_url.password(val);
    }

    std::string get_host() const {
        return std::string{ m_url.host() };
    }
    void set_host(const std::string& val) {
        m_url.host(val);
    }

    std::string get_hostname() const {
        return std::string{ m_url.hostname() };
    }
    void set_hostname(const std::string& val) {
        m_url.hostname(val);
    }

    std::string get_port() const {
        return std::string{ m_url.port() };
    }
    void set_port(const std::string& val) {
        m_url.port(val);
    }

    std::string get_path() const {
        return std::string{ m_url.path() };
    }

    std::string get_pathname() const {
        return std::string{ m_url.pathname() };
    }
    void set_pathname(const std::string& val) {
        m_url.pathname(val);
    }

    std::string get_search() const {
        return std::string{ m_url.search() };
    }
    void set_search(const std::string& val) {
        m_url.search(val);
    }

    std::string get_hash() const {
        return std::string{ m_url.hash() };
    }
    void set_hash(const std::string& val) {
        m_url.hash(val);
    }

    bool valid() const {
        return m_url.is_valid();
    }
    bool base_valid() const {
        return m_base_valid;
    }
private:
    upa::url m_url;
    bool m_base_valid;
};


// Binding code
EMSCRIPTEN_BINDINGS(url_api) {
    class_<URL>("URL")
        .constructor<std::string>()
        .constructor<std::string, std::string>()
        .property("href", &URL::get_href, &URL::set_href)
        .property("origin", &URL::get_origin)
        .property("protocol", &URL::get_protocol, &URL::set_protocol)
        .property("username", &URL::get_username, &URL::set_username)
        .property("password", &URL::get_password, &URL::set_password)
        .property("host", &URL::get_host, &URL::set_host)
        .property("hostname", &URL::get_hostname, &URL::set_hostname)
        .property("port", &URL::get_port, &URL::set_port)
        .property("path", &URL::get_path)
        .property("pathname", &URL::get_pathname, &URL::set_pathname)
        .property("search", &URL::get_search, &URL::set_search)
        .property("hash", &URL::get_hash, &URL::set_hash)
        // is valid?
        .property("valid", &URL::valid)
        .property("base_valid", &URL::base_valid);
}
