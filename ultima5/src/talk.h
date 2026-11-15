#ifndef _TALK_H
#define _TALK_H

int TALK_031e_TalkToNpc(int param_1);
#if !defined(TARGET_DOS16)
void TALK_TestMerchant(int param_1); // test hook: start a shop conversation without walking to the merchant
#endif
int TALK_041c_TalkCmd(void);

#endif
