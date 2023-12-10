var A[10];

main()
{
	var i;
	for(i = 0; i < 10; i = i + 1) A[i] = i;
	println("s = %d",arraySum(A,10));
}

arraySum(a,n)
{
	var i,s;
	s = 0;
	for(i = 0; i < 10; i = i + 1) s = s + a[i];
	return s;
}

