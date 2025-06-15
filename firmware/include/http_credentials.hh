#pragma once

struct HttpCredentials
{
    static constexpr auto MAX_USERNAME_LENGTH = 32;
    static constexpr auto MAX_PASSWORD_LENGTH = 32;

    char username[MAX_USERNAME_LENGTH + 1] = {};
    char password[MAX_PASSWORD_LENGTH + 1] = {};
};
