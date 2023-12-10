
multiply(j)
{
	int i,f;
	f = 1;

	i = 1;
	while ( i < j)
	{
		f = f * i;
		i = i + 1;
	}

	return f;
}

factorial(n)
{
	int f;
	f = multiply(n);
	return f;
}

main()
{
	int i;
	i = 1;

	while(i < 11) {
		int v;
		v = factorial(i);
		println("factorial(%d)\n", v);
		i = i + 1;
	}

	return 0;
}

