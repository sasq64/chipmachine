
		AUTO	wb\VolList\VolListEnd

VolList
Volume		set	0

		rept	65

Value		set	0
Sample		set	0

		rept	128
		dc.b	Sample
Value		set	Value+1
Sample		set	(Value*Volume/64)/2
		endr

Value		set	-128
Sample		set	(Value*Volume/64)/2

		rept	128
		dc.b	Sample
Value		set	Value+1
Sample		set	(Value*Volume/64)/2
		endr

Volume		set	Volume+1
		endr

VolListEnd
