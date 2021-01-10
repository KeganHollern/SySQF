diag_log "test!";

_filename = "read_me.txt";
_mode = "r";
_handle = openFile [_filename, _mode];
if(_handle != 0) then {
	diag_log "handle open!";
	_content = [];
	_c = 1;
	while{_c != 0} do {
		diag_log "reading...";
		_c = readChar _handle;
		diag_log str(_c);
		_content pushback _c;
		if(isEndOf _handle) exitWith {};
	};
	closeFile _handle;
	diag_log toString _content;
} else {
	diag_log "Failed to open file!";
};