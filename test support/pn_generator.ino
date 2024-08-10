uint32_t previous_time {0};
uint8_t pinstate = LOW;
uint16_t shift_register {0x03FF};
uint8_t xor_out;

void setup()
{
	//Serial.begin(115200);

    //while (!Serial);
    
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
}

void loop()
{
    uint32_t current_time = micros();
    if (current_time - previous_time >= 50)
    {
        previous_time = micros();
        if (pinstate == HIGH)
        {
            pinstate = LOW;
        }
        else
        {
            /*  the method with easy to understand steps
            pinstate = HIGH;
            //Serial.print("sr value: ");Serial.println(shift_register, BIN);
            shift_register = shift_register << 1;
            //Serial.print("sr value after shift: ");Serial.println(shift_register, BIN);
            int shift_9 = shift_register >> 9; //move to bit 0
            //Serial.print("bit 9 value after shift: ");Serial.println(shift_9, BIN);
            //int shift_5 = shift_register >> 5; //move to bit 0
            //Serial.print("bit 5 value after shift: ");Serial.println(shift_5, BIN);
            int bit_5 = shift_5 & 0x01;  //mask out all but bit 1
            int bit_9 = shift_9 & 0x01; //mask out all but bit 1
            //Serial.print("masked bit 5 value: ");Serial.println(bit_5, BIN);
            //Serial.print("masked bit 9 value: ");Serial.println(bit_9, BIN);
            xor_out = bit_5 ^ bit_9;
            //Serial.print("xor out: "); Serial.println(xor_out, BIN);
            shift_register = shift_register | xor_out;
            int pn_out = (shift_register >> 9) & 0x01;
            //Serial.print("pn out: "); Serial.println(pn_out, BIN);
            digitalWrite(3, pn_out);
            */

            // shorter number of instructions
            /*
            pinstate = HIGH;
            shift_register = shift_register << 1;
            uint16_t shift_9 = shift_register >> 4; //move to bit 5, so that they align
            xor_out = ((shift_9 ^ shift_register) >> 5) & 0x01;  shift result to bit 0 and mask it
            shift_register = shift_register | xor_out;  //put the result into bit 0 for the next time
            digitalWrite(3, xor_out);
            */

            // shorter sequence to make it easier to check
            pinstate = HIGH;
            shift_register = shift_register << 1;
            uint16_t shift_4 = shift_register >> 1; //move to bit 3, so that they align
            xor_out = ((shift_4 ^ shift_register) >> 3) & 0x01;
            shift_register = shift_register | xor_out;  //put the result into bit 0 for the next time
            digitalWrite(3, xor_out);

        }
        digitalWrite(2, pinstate);
    }

	
}
