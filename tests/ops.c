main(argc, argv)
{
	int mul;
	int a;
	int b;
	a = 32;
	b = 2;
	mul = a * b;

#if 0
    printf("*  res: %d\n", 32 * 2);
    printf("/  res: %d\n", 32 / 2);
    printf("+  res: %d\n", 32 + 2);
    printf("-  res: %d\n", 32 - 2);
    printf("%  res: %d\n", 32 % 2);
    printf(">> res: %d\n", 32 >> 2);
    printf("<< res: %d\n", 32 << 2);
    printf("<  res: %d\n", 32 < 2);
    printf("<= res: %d\n", 32 <= 2);
    printf(">  res: %d\n", 32 > 2);
    printf("=> res: %d\n", 32 >= 2);
    printf("== res: %d\n", 32 == 2);
    printf("!= res: %d\n", 32 != 2);
    printf("&  res: %d\n", 32 & 2);
    printf("^  res: %d\n", 32 ^ 2);
    printf("|  res: %d\n", 32 | 2);
    printf("&& res: %d\n", 32 && 2);
    printf("|| res: %d\n", 32 || 2);
#endif
    return 0;
}

