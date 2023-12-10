
exp(b, x)
{
	int r, i;
	r = 1;
	i = 0;

	while (i < x) {
		r = r * b;
		i = i + 1;
	}
	return r;
}

fac(i, j)
{
	int p;
	p = 1;

	i = 1;
	while (i < j) {
		p = p * i;
		i = i + 1;
	}
	return p;
}

rfac(n)
{
	if (n < 1)
		return 1;
	return n * rfac(n-1);
}


main()
{
	int i;
	i = 2;
	while (i < 10) {
		int p;
		p = fac(2, i);
		println("factorial(%d)", p);
		i = i + 1;
	}
	i = exp(2, 27);
	println("2 ^ 27 (%d)", i);

	i = rfac(4);
	println("rfac %d", i);
	return 0;
}

