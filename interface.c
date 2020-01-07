#include "interface.h"
#include "buttons.h"
#include "lcd.h"
#include "memory/kblayout.h"

void LCD_AutoDrawSprite_P(int8_t PosX, int8_t PosY, const uint8_t* Buffer, int8_t Width, int8_t Height)
{
	LCD_DrawSprite_P(PosX, PosY, Buffer, Width, Height);
	LCD_Render();
}

uint8_t Interface_Keyboard_P(char* Buffer, uint8_t Max, const char* Title)
{
	Max--;
	int8_t x = 0, y = 1;
	uint8_t Shift = 0;
	uint8_t Length = 0;
	Buffer[0] = '\0';
	while(!Button_Return_Pressed())
	{
		LCD_Clear();
		uint8_t i;
		LCD_DrawText(0, 0, &Buffer[(Length > 14) ? Length-14 : 0]);
		LCD_DrawText_P(0, 1, Title);
		for(i = 0; i<14; i++)
		LCD_InvertCell(i, 1);
		if(!Shift)
			for(i = 0; i < 4; i++)
				LCD_DrawText_P(0, 2+i, KbLayout[i]);
		else
			for(i = 0; i < 4; i++)
				LCD_DrawText_P(0, 2+i, KbLayoutShift[i]);
		LCD_InvertCell(x, 2+y);
		if(Shift&SHIFTMODE_ONCE)
			LCD_InvertCell(0, 5);
		if(Shift&SHIFTMODE_ON)
			LCD_InvertCell(0, 4);
		LCD_Render();

		if(Button_Down_Pressed())
			if(y < 3)
				y++;
		if(Button_Right_Pressed())
			if(x < 13)
				x++;
		if(Button_Up_Pressed())
			if(y > 0)
				y--;
		if(Button_Left_Pressed())
			if(x > 0)
				x--;
		if(Button_OK_Pressed())
		{
			char Ch = pgm_read_byte(Shift ? &KbLayoutShift[y][x] : &KbLayout[y][x]);
			if(Ch >= ' ' && Length < Max)
			{
				Buffer[Length] = Ch;
				Buffer[Length+1] = '\0';
				Length++;
				if(Shift&SHIFTMODE_ONCE)
				Shift = 0;
			}
			else
			{
				switch(Ch)
				{
					case 0x1:
						if(Length > 0)
						{
							Length--;
							Buffer[Length] = '\0';
						}
						break;
					case 0x2:
						if(!Shift)
							Shift |= SHIFTMODE_ON;
						else
							Shift = 0;
						break;
					case 0x3:
						if(!Shift)
							Shift |= SHIFTMODE_ONCE;
						else
							Shift = 0;
						break;
					case 0x4:
						return Length;
						break;
				}
			}
		}
	}
	Buffer[0] = '\0';
	return 0;
}

inline static const TMenuEntry* getMenuEntry(const TMenu* Menu, uint8_t i)
{
	return (const TMenuEntry*)pgm_read_ptr(&Menu->Entries[i]);
}

void Interface_Menu_P(const TMenu* Menu)
{
	int8_t Selection = 0;
	int8_t View = Selection;
	uint8_t NumEntries =  pgm_read_byte(&Menu->NumEntries);
	while(1)
	{
		LCD_Clear();
		LCD_DrawText_P(0, 0, pgm_read_ptr(&Menu->Title));
		for(uint8_t i = 0; i<((NumEntries>4) ? 4 : NumEntries); i++)
			LCD_DrawText_P(0, i+2, pgm_read_ptr(&getMenuEntry(Menu, i+View)->Text ) );
		for(uint8_t i = 0; i<14; i++)
			LCD_InvertCell(i, (Selection-View)+2);
		LCD_Render();
		
		while(1)
		{
			if(Button_Up_Pressed())
				if(Selection > 0)
				{
					Selection--;
					break; // or inputLoop = false;
				}
			if(Button_Down_Pressed())
				if(Selection < NumEntries-1)
				{
					Selection++;
					break; // inputLoop = false;
				}
			if(Button_OK_Pressed())
			{
				//const TFunction* Callback = (const TFunction*)pgm_read_ptr(&getMenuEntry(Menu, Selection)->Param );
				if(pgm_read_ptr(&getMenuEntry(Menu, Selection)->Param) != 0 && (uint8_t)pgm_read_byte(&getMenuEntry(Menu, Selection)->Flags)==(MENU_ENTRY_FUNCTION|MENU_ENTRY_PROGMEM))
				{
					//Callback->Function();
					void (*Function)() = pgm_read_ptr(&((const TFunction*)pgm_read_ptr(&getMenuEntry(Menu, Selection)->Param))->Function);
					Function();
				}
				break;
			}
			//if(Button_Return_Pressed())
			//	return;
		}
		
		if(View+3 < Selection) // follow the lower selection
			View++;
		if(Selection < View) // follow the upper selection
			View--;
		
		
	}
}

void Interface_Scrollbar_P(const char* Name, int8_t* Value, int8_t Bottom, int8_t Top, int8_t Step)
{
	int8_t Prev = *Value;
	while(1)
	{
		if(*Value > Top)
			*Value = Top;
		if(*Value < Bottom)
			*Value = Bottom;
		
		LCD_Clear();
		LCD_DrawText_P(6, 1, Name);
		LCD_SetDramByte(6, 4, 0xff); // scrollbar startpos
		uint8_t i = 1;
		const uint16_t Fill = 70*(*Value-Bottom)/(Top-Bottom); // why 70? -> 84 - display width, 6 - cell width, 72 - scrollbar total width, 70 - scrollbar fill area
		for(; i<Fill; i++)
			LCD_SetDramByte(i+6, 4, 0xff); // fill
		for(; i<70; i++)
			LCD_SetDramByte(i+6, 4, 0x81); // empty;
		LCD_SetDramByte(76, 4, 0xff); // finish border
		//LCD_DrawText() // display current value
		LCD_Render();
		
		while(1)
		{
			if(Button_Return_Pressed())
			{
				*Value = Prev;
				return;	
			}
			if(Button_OK_Pressed())
				return;
			if(Button_Left_Pressed())
			{
				*Value-=Step;
				break;
			}
			if(Button_Right_Pressed())
			{
				*Value+=Step;
				break;
			}
		}
	}
}

void Interface_Switch_P(const char* Switches, uint8_t* Selection) // maybe return int instead of using this pointer
{
	const char* chStart = Switches;
	uint8_t NumSwitches = 1;
	uint8_t Prev = *Selection;
	while(*chStart)
	{
		if(*chStart == '\r')
		{
			NumSwitches++;
		}
	}
	chStart = 0;
	while(1)
	{
		if(!chStart)
		{
			chStart = Switches;
			for(uint8_t i = 0; i!=*Selection; chStart++)
			{
				if(*chStart == '\r')
					i++;
				if(!*chStart)
					return; // bad selection
			}
		}
		LCD_Clear();
		LCD_DrawText_P(6, 3, chStart);
		//...
		LCD_Render();
		if(Button_Return_Pressed())
		{
			*Selection = Prev;
			return;
		}
		if(Button_OK_Pressed())
			return;
		if(Button_Left_Pressed())
		{
			if(*Selection > 0)
			{
				chStart = 0;
				(*Selection)--;
			}
		}
		if(Button_Right_Pressed())
		{
			if(*Selection < NumSwitches-1)
			{
				chStart = 0;
				(*Selection)++;
			}
		}
	}
	
}