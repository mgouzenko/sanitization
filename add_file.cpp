#include <cctype>
#include <exception>
#include <iostream>
#include <string>

using namespace std;

struct parse_error: public runtime_error{
	parse_error(const string err): runtime_error::runtime_error(err){};
};

string verify_string(string s){
	for(size_t i = 0; i < s.length(); i++){
		if(!isgraph(s[i]))
			throw parse_error(string("Invalid character") + s[i]);
	}
	return s;
}

bool verify_name(string name){
	cout << name;
	return true;
}

bool verify_data(string data){
	cout << data;
	return true;
}

int count_slashes(string line, size_t idx){
	int count = 0;
	while(line[idx] == '\\'){
		count++;
		if(idx == 0) break;
		else idx--;
	}
	return count;
}

pair<string, string> parse_line(string line){
	if(line.empty()) throw parse_error("Empty line.");

	string ending = " \t";
	size_t begin_idx = 0;
	size_t end_idx = 0;
	if(line[0] == '\"' || line[0] == '\''){
		ending = line[0];
		begin_idx = 1;
	}

	for(;;){
		// Advance the end_idx until we find a char that might mark the end.
		// This may be a space, tab, or quote (if the first char was a quote).
		end_idx = line.find_first_of(ending, end_idx+1);

		// If the appropriate ending cannot be found, abort
		if(end_idx == string::npos){
			if(begin_idx)
				throw parse_error(string("Unmatched \"") + line[0] + string("\""));
			else
				throw parse_error("No data field found");
		}

		// Two scenarios:
		//
		// 1) We're not looking for a closing quote. If that's the case, begin_idx
		// is 0. So, if begin_idx is 0, we can break.
		else if(!begin_idx) break;

		// 2) We're looking for a closing quote. If that's the case, we must also
		// ensure that the prior characters do not consist of an odd number of
		// backslashes.
		else if(begin_idx && (count_slashes(line, end_idx-1) % 2 == 0)){

			// We also need there to be a space after the quote.
			size_t next_char_idx = end_idx + 1;
			if(next_char_idx == line.length()||
				((line[next_char_idx] != ' ') && (line[next_char_idx] != '\t') ))
				throw parse_error("There must be a space after the name.");
			break;
		}
	}

	// begin_idx should now point to the first non-quote char in the string.
	// end_idx should now point to the space or tab after the string.
	// Use these indexes to extract the name of the file.
	string name = line.substr(begin_idx, (end_idx - begin_idx));

	if(begin_idx) end_idx++;

	// Eat the whitespace
	while(line[end_idx] == ' ' || line[end_idx] == '\t'){

		// If we ever reach the last character and we're not done
		// eating whitespace, that means the data was never supplied.
		if(end_idx++ == line.length()-1)
			throw parse_error("Could not find data field");
	}

	// By now, end_idx should be pointing to the beginning of the data.
	string data;

	// The first character might be a quote. Skip it.
	if(line[end_idx] == '\'' || line[end_idx] == '\"'){
		// begin_idx now points to the first real character in the data.
		cout << end_idx;
		begin_idx = end_idx+1;
		ending = line[end_idx];

		// Find matching quote
		for(;;){
			end_idx = line.find_first_of(ending, end_idx+1);

			// We could not find the matching quote
			if(end_idx == string::npos)
				throw parse_error("Unmatched \"" + ending + "\"");

			// We did find a matching quote, AND it wasn't part of an escape sequence.
			else if(count_slashes(line, end_idx-1) % 2 == 0){

				// There can only be spaces after the quote.
				// If there's anything else, abort.
				size_t next = end_idx + 1;
				if(next == line.length()||
				   line[next] == ' ' ||
				   line[next] == '\t') break;
				else throw parse_error("Invalid characters after data field");
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
	if(end_idx == string::npos) end_idx = line.length();

	data = line.substr(begin_idx, (end_idx-begin_idx));

	while(++end_idx < line.length())
		if(line[end_idx] != ' ' && line[end_idx] != '\t')
			throw parse_error("Too many arguments");

	return make_pair(name, data);
}

int main(){
	string line;
	while(getline(cin, line)){
		auto name_and_data = parse_line(line);
		cout << name_and_data.first << endl <<
		name_and_data.second << endl;
	}
}
