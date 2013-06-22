	open "ram:test" for output as 1
	A=0
	B=65/128
	for X=1 to 16
	print #1,"		dc.b	";
	for I=1 to 8
	print #1,fix (A+0.5);
	A=A+B
	if I=8 then Skip
	print #1,",";
Skip:	next I
	print #1,""
	next X
	close #1
