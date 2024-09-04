

constexpr bool validate_utf8(const char* string) noexcept
{
    while (*string)
    {
        if ((*string & 0b10000000) != 0)
        {
            if ((*string & 0b01000000) == 0) return false;
            if ((*string & 0b00100000) != 0)
            {
                if ((*string & 0b00010000) != 0)
                {
                    if ((*string & 0b00001000) != 0)
                        return false;

                    if ((*++string & 0b11000000) != 0b10000000)
                        return false;
                }

                if ((*++string & 0b11000000) != 0b10000000)
                    return false;
            }

            if ((*++string & 0b11000000) != 0b10000000)
                return false;
        }

        ++string;
    }

    return true;
}