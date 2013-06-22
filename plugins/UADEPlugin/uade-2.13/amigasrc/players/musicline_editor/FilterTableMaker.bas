	open "ram:test" for output as 1
	A=2048*1.010889287
	for X=1 to 256/8
	print #1,"		dc.w	";
	for I=1 to 8
	print #1,fix (A+0.5);
	if I=8 then Skip
	print #1,",";
Skip:	A=A*1.010889287
	next I
	print #1,""
	next X
	close #1
