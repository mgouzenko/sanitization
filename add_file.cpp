#include <cctype>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

int MAX_PATH_LENGTH = 300;
using namespace std;

struct parse_error : public runtime_error {
  parse_error(const string err) : std::runtime_error::runtime_error(err){};
};

bool is_international(char c) {
  if (c == '\203' || c == '\212' || c == '\214' || c == '\216' || c == '\232' ||
      c == '\234' || c == '\236' || c == '\237' || c == '\252' || c == '\262' ||
      c == '\263' || c == '\271' || c == '\272' ||
      (c >= '\300' && c <= '\326') || (c >= '\330' && c <= '\366') ||
      (c >= '\370' && c <= '\377'))
    return true;
  return false;
}

bool is_other(char c) {
  if (c != '\201' && c != '\215' && c != '\217' && c != '\220' && c != '\235')
    return true;
  else
    return false;
}

bool is_valid_char(char c, bool quoted) {
  if (c == '\0')
    return false;
  if (quoted) {
    if (isalnum(c) || isspace(c) || is_international(c) || ispunct(c) ||
        is_other(c))
      return true;
  } else {
    if (isalnum(c) || is_international(c))
      return true;
  }
  return false;
}

string escape(string s, bool quoted) {
  if (!quoted) {
    for (size_t i = 0; i < s.length(); i++) {
      if (!is_valid_char(s[i], false))
        throw parse_error("Invalid character");
    }
    return s;
  } else {
    string escaped_string = "";
    for (size_t i = 0; i < s.length(); i++) {
      if (s[i] != '\\') {
        if (!is_valid_char(s[i], true))
          throw parse_error("Invalid character");
        escaped_string += s[i];
        continue;
      } else if (i == s.length() - 1) {
        throw parse_error("Invalid escape sequence.");
      } else if (isdigit(s[i + 1])) {
        string octal_digits = "";
        int digits = 0;
        while (isdigit(s[i + 1]) && ++digits != 4)
          octal_digits += s[++i];
        if (digits != 3)
          throw parse_error("Not enough octal digits");
        auto char_code = strtol(octal_digits.c_str(), NULL, 8);
        if (char_code > 255 || char_code <= 0 ||
            !is_valid_char((char)char_code, true))
          throw parse_error("Invalid octal integer.");
        else {
          escaped_string += (char)char_code;
        }
        continue;
      } else if (s[i + 1] == 'n') {
        escaped_string += '\n';
      } else if (s[i + 1] == 't') {
        escaped_string += '\t';
      } else if (s[i + 1] == 'r') {
        escaped_string += '\r';
      } else if (s[i + 1] == '\\') {
        escaped_string += '\\';
      } else if (s[i + 1] == '\'') {
        escaped_string += '\'';
      } else if (s[i + 1] == '\"') {
        escaped_string += '\"';
      } else {
        throw parse_error("Invalid escape sequence");
      }
      i++;
    }
    return escaped_string;
  }
}

string normalize_name(string name) {
  string normalized_name = "";
  name += ".mag2272";
  char cwdbuffer[MAX_PATH_LENGTH];
  string cwd = getcwd(cwdbuffer, MAX_PATH_LENGTH);

  if (name.find("//") != string::npos)
    throw parse_error("Bad file name");

  int start_idx = 0;
  if (name[0] != '/') {
    name = cwd + '/' + name;
  }

  vector<string> components;
  stringstream filename_stream(name.substr(start_idx));
  string component;

  getline(filename_stream, component, '/');
  while (getline(filename_stream, component, '/')) {
    if (component == "..") {
      if (!components.empty())
        components.pop_back();
    } else {
      components.push_back(component);
    }
  }

  for (size_t i = 0; i < components.size() - 1; i++)
    normalized_name = normalized_name + '/' + components[i];

  if (normalized_name != cwd && normalized_name != "/tmp") {
    throw runtime_error("Can only write to files in /tmp or CWD.");
  }

  normalized_name += ('/' + components.back());

  return normalized_name;
}

string sanitize(string s) {
  string sanitized_string = "";
  for (size_t i = 0; i < s.length(); i++) {
    if (s[i] == '\'')
      sanitized_string += "\'\\\'\'";
    else
      sanitized_string += s[i];
  }

  return sanitized_string;
}

int count_slashes(string line, size_t idx) {
  int count = 0;
  while (line[idx] == '\\') {
    count++;
    if (idx == 0)
      break;
    else
      idx--;
  }
  return count;
}

pair<string, string> parse_line(string line) {
  if (line.empty())
    throw parse_error("Empty line.");
  bool is_name_quoted = false;
  bool is_data_quoted = false;

  string ending = " \t";
  size_t begin_idx = 0;
  size_t end_idx = 0;
  if (line[0] == '\"' || line[0] == '\'') {
    ending = line[0];
    begin_idx = 1;
    is_name_quoted = true;
  }

  for (;;) {
    // Advance the end_idx until we find a char that might mark the end.
    // This may be a space, tab, or quote (if the first char was a quote).
    end_idx = line.find_first_of(ending, end_idx + 1);

    // If the appropriate ending cannot be found, abort
    if (end_idx == string::npos) {
      if (begin_idx)
        throw parse_error(string("Unmatched \"") + line[0] + string("\""));
      else
        throw parse_error("No data field found");
    }

    // Two scenarios:
    //
    // 1) We're not looking for a closing quote. If that's the case, begin_idx
    // is 0. So, if begin_idx is 0, we can break.
    else if (!begin_idx)
      break;

    // 2) We're looking for a closing quote. If that's the case, we must also
    // ensure that the prior characters do not consist of an odd number of
    // backslashes.
    else if (begin_idx && (count_slashes(line, end_idx - 1) % 2 == 0)) {

      // We also need there to be a space after the quote.
      size_t next_char_idx = end_idx + 1;
      if (next_char_idx == line.length() ||
          ((line[next_char_idx] != ' ') && (line[next_char_idx] != '\t')))
        throw parse_error("There must be a space after the name.");
      break;
    }
  }

  // begin_idx should now point to the first non-quote char in the string.
  // end_idx should now point to the space or tab after the string.
  // Use these indexes to extract the name of the file.
  string name = line.substr(begin_idx, (end_idx - begin_idx));

  if (begin_idx)
    end_idx++;

  // Eat the whitespace
  while (line[end_idx] == ' ' || line[end_idx] == '\t') {

    // If we ever reach the last character and we're not done
    // eating whitespace, that means the data was never supplied.
    if (end_idx++ == line.length() - 1)
      throw parse_error("Could not find data field");
  }

  // By now, end_idx should be pointing to the beginning of the data.
  string data;

  // The first character might be a quote. Skip it.
  if (line[end_idx] == '\'' || line[end_idx] == '\"') {
    // begin_idx now points to the first real character in the data.
    begin_idx = end_idx + 1;
    ending = line[end_idx];
    is_data_quoted = true;

    // Find matching quote
    for (;;) {
      end_idx = line.find_first_of(ending, end_idx + 1);

      // We could not find the matching quote
      if (end_idx == string::npos)
        throw parse_error("Unmatched \"" + ending + "\"");

      // We did find a matching quote, AND it wasn't part of an escape sequence.
      else if (count_slashes(line, end_idx - 1) % 2 == 0) {

        // There can only be spaces after the quote.
        // If there's anything else, abort.
        size_t next = end_idx + 1;
        if (next == line.length() || line[next] == ' ' || line[next] == '\t')
          break;
        else
          throw parse_error("Invalid characters after data field");
      }
    }
  }

  // It might be the case that there's only a space.
  else {
    begin_idx = end_idx;
    end_idx = line.find_first_of(' ', end_idx);
  }

  // begin_idx should point to the first character of the data.
  // end_idx should either point to one past the data or is npos.
  if (end_idx == string::npos)
    end_idx = line.length();

  data = line.substr(begin_idx, (end_idx - begin_idx));

  while (++end_idx < line.length())
    if (line[end_idx] != ' ' && line[end_idx] != '\t')
      throw parse_error("Too many arguments");

  auto escaped_name = escape(name, is_name_quoted);
  auto escaped_data = escape(data, is_data_quoted);
  return make_pair(escaped_name, escaped_data);
}

int main() {
  // istringstream instream("ab\347 123\n"
  //					   "\"a\\\"b\\143\" 123\n"
  //					   "\"/tmp/meow\" cat\n"
  //					   "gotcha \" \\\"; echo gotcha; echo \\\"
  //\"\n");
  string line;
  while (getline(cin, line)) {
    try {
      auto name_and_data = parse_line(line);
      string normalized_name = normalize_name(name_and_data.first);
      string sanitized_name = sanitize(normalized_name);
      string sanitized_data = sanitize(name_and_data.second);
      string cmd =
          "echo \'" + sanitized_data + "\' >> " + '\'' + sanitized_name + '\'';
      system(cmd.c_str());
    } catch (runtime_error e) {
    }
  }
}
