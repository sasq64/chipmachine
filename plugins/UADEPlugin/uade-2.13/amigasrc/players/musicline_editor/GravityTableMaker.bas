	open "ram:test" for output as 1
	A#=28160:B#=880:C#=32*12*6
	D#=(A#/B#)^(1/C#)
	print D#
	for X=1 to (C#+1)/8+1
	print #1,"		dc.w	";
	for I=1 to 8
	print #1,cint (B#);
	if I=8 then Skip
	print #1,",";
Skip:	B#=B#*D#
	next I
	print #1,""
	next X
	close #1
