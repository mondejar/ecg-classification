	integer i, v(2), g

	i = isigopen("100s"//CHAR(0), 2)
	do i = 1, 10
	 g = getvec(v)
	 write (6,3) v(1), v(2)
 3	format("v(1) = ", i4, "    v(2) = ", i4)
	end do
	end
