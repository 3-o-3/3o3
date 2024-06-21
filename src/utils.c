

// unsigned  ==
int u_eq(int a, int b)
{
    return a == b;
}

// unsigned <
int u_lt(int a, int b)
{
    if (b == a)
    {
        return 0;
    }
    if (a < 0)
    {
        if (b < 0)
        {
            // FF  80 -> -1 -128
            return a < b;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        if (b < 0)
        {
            return 0;
        }
        else
        {
            return a < b;
        }
    }
}

// unsigned <=
int u_le(int a, int b)
{
    if (a == b)
    {
        return 1;
    }
    return u_lt(a, b);
}

// unsigned >=
int u_ge(int a, int b)
{
    if (a == b)
    {
        return 1;
    }
    return !u_lt(a, b);
}

// unsigned >
int u_gt(int a, int b)
{
    if (a == b)
    {
        return 0;
    }
    return !u_lt(a, b);
}

int i64_add(int *dest, int *a, int *b)
{
    int d0, d1, o, c;

    d0 = a[0];
    d1 = a[1];

    o = (a[0] & 0xFFFF) + (b[0] & 0xFFFF);
    c = o & 0xFFFF0000;
    d0 = o & 0xFFFF;
    o = ((a[0] >> 16) & 0xFFFF) + ((b[0] >> 16) & 0xFFFF) + (c >> 16);
    c = o & 0xFFFF0000;
    d0 |= (o << 16);

    o = (a[1] & 0xFFFF) + (b[1] & 0xFFFF);
    c = o & 0xFFFF0000;
    d1 = o & 0xFFFF;
    o = ((a[1] >> 16) & 0xFFFF) + ((b[1] >> 16) & 0xFFFF) + (c >> 16);
    c = o & 0xFFFF0000;
    d1 |= (o << 16);

    dest[0] = d0;
    dest[1] = d1;
    return d0;
}
