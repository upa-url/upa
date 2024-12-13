// Copyright 2024 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.
//

#include "upa/public_suffix_list.h"

namespace upa {
namespace {

// utilities

class splitter {
public:
    splitter(std::string_view domain);

    void start();
    bool next(std::string& label);
    bool next(std::string_view& label);
    std::size_t index() const {
        return label_ind_;
    }
    bool at_begin() const {
        return label_ind_ == label_pos_.size() - 1;
    }
    bool at_end() const {
        return label_ind_ == 0;
    }

    std::size_t size() const {
        return label_pos_.size();
    }
    std::string get_str(std::size_t ind) const {
        return std::string{ domain_.substr(label_pos_[ind]) };
    }

private:
    const std::string_view domain_;
    std::vector<std::size_t> label_pos_;

    std::size_t label_end_ = 0;
    std::size_t label_ind_ = 0;
};

inline splitter::splitter(std::string_view domain)
    : domain_{ domain }
    , label_end_{ domain_.length() }
{
    label_pos_.reserve(16);
    label_pos_.push_back(0);
    std::size_t pos = 0;
    while ((pos = domain_.find('.', pos)) != std::string::npos)
        label_pos_.push_back(++pos); // skip '.' and add pos
    label_ind_ = label_pos_.size();
}

inline void splitter::start() {
    label_end_ = domain_.length();
    label_ind_ = label_pos_.size();
}

inline bool splitter::next(std::string& label) {
    if (label_ind_ != 0) {
        const auto pos = label_pos_[--label_ind_];
        label = domain_.substr(pos, label_end_ - pos);
        label_end_ = pos - 1; // skip '.'
        return true;
    }
    return false;
}

inline bool splitter::next(std::string_view& label) {
    if (label_ind_ != 0) {
        const auto pos = label_pos_[--label_ind_];
        label = domain_.substr(pos, label_end_ - pos);
        label_end_ = pos - 1; // skip '.'
        return true;
    }
    return false;
}

constexpr void trim_dots(std::string_view& sv) {
    const auto* first = sv.data();
    const auto* last = first + sv.length();
    while (first != last && *first == '.') ++first;
    while (first != last && *(last - 1) == '.') --last;
    sv = std::string_view{ first, static_cast<std::size_t>(last - first) };
}

} // namespace

// class public_suffix_list

bool public_suffix_list::load(std::istream& input_stream) {
    push_context ctx;

    std::string line;
    while (std::getline(input_stream, line))
        push_line(ctx, line);
    return ctx.code_flags == 0;
}

void public_suffix_list::push_line(push_context& ctx, std::string_view line) {
    static constexpr auto insert = [](label_item& root, std::string_view input, std::uint8_t code) {
        // TODO: maybe only to Punycode
        std::string domain = upa::url_host{ input }.to_string();

        splitter labels(domain);
        label_item* pli = &root;
        std::string label;
        while (labels.next(label)) {
            if (!pli->children)
                pli->children = std::make_unique<label_item::map_type>();
            if (labels.at_end())
                (*pli->children)[label].code = code;
            else
                pli->children->emplace(label, label_item{});
            pli = &(*pli->children)[label];
        }
    };

    if (line.empty())
        return;
    if (line.length() >= 2) {
        if (line[0] == '/' && line[1] == '/') {
            if (line == "// ===BEGIN ICANN DOMAINS===")
                ctx.code_flags = label_item::IS_ICANN;
            else if (line == "// ===END ICANN DOMAINS===")
                ctx.code_flags = 0;
            else if (line == "// ===BEGIN PRIVATE DOMAINS===")
                ctx.code_flags = label_item::IS_PRIVATE;
            else if (line == "// ===END PRIVATE DOMAINS===")
                ctx.code_flags = 0;
            return;
        }
        if (line[0] == '*' && line[1] == '.') {
            insert(root_, line.substr(2), 3 | ctx.code_flags);
            return;
        }
    }
    if (line[0] == '!')
        insert(root_, line.substr(1), 1 | ctx.code_flags);
    else
        insert(root_, line, 2 | ctx.code_flags);

}

void public_suffix_list::push(push_context& ctx, std::string_view buff) {
    std::size_t sol = 0;
    if (!ctx.remaining.empty()) {
        auto eol = buff.find('\n', 0);
        ctx.remaining += buff.substr(0, eol);
        if (eol == buff.npos)
            return;
        push_line(ctx, ctx.remaining);
        ctx.remaining.clear();
        sol = eol + 1; // skip '\n'
    }
    while (sol < buff.size()) {
        auto eol = buff.find('\n', sol);
        if (eol == buff.npos) {
            ctx.remaining = buff.substr(sol);
            return;
        }
        push_line(ctx, buff.substr(sol, eol - sol));
        sol = eol + 1; // skip '\n'
    }
}

bool public_suffix_list::finalize(push_context& ctx) {
    if (!ctx.remaining.empty()) {
        push_line(ctx, ctx.remaining);
        ctx.remaining.clear();
    }
    return ctx.code_flags == 0;
}


std::string public_suffix_list::get_host_suffix(std::string_view hostname, bool reg_domain) const {
    // Definitions. Empty labels are not permitted, meaning that leading
    // and trailing dots are ignored.
    trim_dots(hostname);

    // Split to labels
    splitter labels(hostname);

    const label_item* pli = &root_;
    std::uint8_t latest_code = 0;
    std::size_t latest_ind = 0;
    std::string_view label;
    while (labels.next(label)) {
        if (pli->children) {
#ifdef __cpp_lib_generic_unordered_lookup
            auto it = pli->children->find(label);
#else
            auto it = pli->children->find(std::string{ label });
#endif
            if (it != pli->children->end()) {
                if (it->second.code && (
                    (it->second.code & label_item::DIFF_MASK) != 3 || !labels.at_end())) {
                    latest_code = it->second.code;
                    latest_ind = labels.index();
                }
                pli = &it->second;
                continue;
            }
        }
        if (labels.at_begin()) {
            // Unlisted TLD: If no rules match, the prevailing rule is "*"
            latest_code = 2;
            latest_ind = labels.index();
        }
        break;
    }
    if (latest_code) {
        const int ind_diff = static_cast<int>(latest_code & label_item::DIFF_MASK) - 2 + static_cast<int>(reg_domain);
        if (ind_diff <= 0 || static_cast<std::size_t>(ind_diff) <= latest_ind)
            return labels.get_str(latest_ind - ind_diff);
    }
    return {};
}

} // namespace upa
