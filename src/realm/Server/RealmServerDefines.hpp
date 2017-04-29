/*
Copyright (c) 2014-2017 AscEmu Team <http://www.ascemu.org/>
This file is released under the MIT license. See README-MIT for more information.
*/

struct AllowedIP
{
    unsigned int IP;
    unsigned char Bytes;
};

enum AccountDatabaseMethod
{
    Method_Account_Ban = 1,
    Method_Account_Set_GM,
    Method_Account_Set_Mute,
    Method_IP_Ban,
    Method_IP_Unban,
    Method_Account_Change_PW,
    Method_Account_Create
};

enum AccountDatabaseResult
{
    Result_Account_PW_wrong = 1,
    Result_Account_SQL_error,
    Result_Account_Finished,
    Result_Account_Exists
};

enum AccountFlags
{
    ACCOUNT_FLAG_VIP = 0x1,
    ACCOUNT_FLAG_NO_AUTOJOIN = 0x2,
    //ACCOUNT_FLAG_XTEND_INFO   = 0x4,
    ACCOUNT_FLAG_XPACK_01 = 0x8,
    ACCOUNT_FLAG_XPACK_02 = 0x10
};

class SERVER_DECL ConfigMgr
{
public:

    ConfigFile MainConfig;
};

extern SERVER_DECL ConfigMgr Config;