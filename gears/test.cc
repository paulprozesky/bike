#include <stdio.h>
#include <string.h>

#define byte unsigned char

const short RATIO_TOLERANCE = 200;  // absolute - unitless

unsigned short ratios[6] = {65535, 65535, 65535, 65535, 65535, 65535};  // store the ratios multiplied by 1000 so we don't need floats
byte num_ratios = 0;
byte MAX_GEARS = 6;

void save_to_eeprom() {
  // fake
}

byte search_and_update(unsigned short new_ratio) {
  /*It wasn't the first one and it wasn't bigger than the biggest, so where is it?
  */
  // find the first time it's smaller than a ratio's upper limit
  unsigned short old_ratios[MAX_GEARS];
  memcpy(old_ratios, ratios, MAX_GEARS*sizeof(ratios[0]));
  byte lower_match = 255;
  for (byte ctr = 0; ctr < num_ratios; ctr++) {
    short this_ratio = ratios[ctr];
    unsigned short this_ratio_upper_limit = this_ratio + RATIO_TOLERANCE;
    unsigned short this_ratio_lower_limit = this_ratio - RATIO_TOLERANCE;
    if ((new_ratio > this_ratio_lower_limit) && (new_ratio <= this_ratio_upper_limit)) {
      // it is this one, do nothing
      return 255;
    }
    else if (new_ratio < this_ratio_lower_limit) {
        lower_match = ctr;
        break;
    }
    else if (new_ratio > this_ratio_upper_limit) {
      // carry on looking
    }
  }
  if (lower_match == 255) {
    // this should never happen
    printf("%s\n", "But somehow it did?");
    return 255;
  }
  // lower chunk has already been copied above
  // move the upper chunk up to make room for the new value
  for(byte ctr = lower_match + 1; ctr < 6; ctr++) {
    ratios[ctr] = old_ratios[ctr - 1];
  }
  // new value
  ratios[lower_match] = new_ratio;
  num_ratios++;
  return lower_match;
}

byte update_gear_table(float ratio) {
  /*Given a ratio that could not be matched to a currently known one, put it in the list.
  */
  // do we have all of them already?
  if (num_ratios >= MAX_GEARS) {
    return 255;
  }
  // otherwise find out where to put this one
  unsigned short new_ratio = ratio * 1000;
  byte new_gear = 255;
  if (num_ratios == 0) {
    // it's the first one
    ratios[0] = new_ratio;
    new_gear = 0;
    num_ratios = 1;
    save_to_eeprom();
    return 0;
  }
  // is it bigger than the currently known biggest one?
  if (new_ratio > (ratios[num_ratios - 1] + RATIO_TOLERANCE)) {
    ratios[num_ratios] = new_ratio;
    new_gear = num_ratios;
    num_ratios++;
  }
  else new_gear = search_and_update(new_ratio);
  if (new_gear < 255) save_to_eeprom();
  return new_gear;
}

void print_ratios() {
  for(int ctr=0; ctr < 6; ctr++) {
    printf("%i: %i\n", ctr, ratios[ctr]);
  }
}

void compare_to_expected(unsigned short *expected_ratios, byte num_expected) {
  byte errors = 0;
  for (byte ctr = 0; ctr < num_expected; ctr++) {
    if (ratios[ctr] != expected_ratios[ctr]) {
      errors++;
    }
  }
  if (errors > 0) {
    printf("ERRORS:\n");
    print_ratios();
  }
}

void reset_ratios() {
  for (byte ctr = 0; ctr < 6; ctr++) ratios[ctr] = 65535;
  num_ratios = 0;
}

void test1(void) {
  printf("Test 1\n");
  reset_ratios();
  update_gear_table(1.5);
  update_gear_table(2.5);
  update_gear_table(0.5);
  update_gear_table(5.5);
  update_gear_table(4.5);
  update_gear_table(3.5);
  unsigned short expected[] = {500, 1500, 2500, 3500, 4500, 5500};
  compare_to_expected(expected, 6);
}

void test2(void) {
  printf("Test 2\n");
  reset_ratios();
  update_gear_table(0.5);
  update_gear_table(1.5);
  update_gear_table(2.5);
  update_gear_table(3.5);
  update_gear_table(4.5);
  update_gear_table(5.5);
  unsigned short expected[] = {500, 1500, 2500, 3500, 4500, 5500};
  compare_to_expected(expected, 6);
}

void test3(void) {
  printf("Test 3\n");
  reset_ratios();
  update_gear_table(0.5);
  update_gear_table(1.5);
  update_gear_table(2.5);
  update_gear_table(3.5);
  update_gear_table(4.5);
  update_gear_table(3.5);
  update_gear_table(2.7);  // 2.5 with noise
  update_gear_table(3.5);
  update_gear_table(3.5);
  update_gear_table(1.4);
  unsigned short expected[] = {500, 1500, 2500, 3500, 4500};
  compare_to_expected(expected, 5);
}

void test4(void) {
  printf("Test 4\n");
  reset_ratios();
  update_gear_table(0.5);
  update_gear_table(1.5);
  update_gear_table(2.5);
  update_gear_table(3.5);
  update_gear_table(4.5);
  update_gear_table(5.5);
  update_gear_table(7.5);  // should be ignored
  update_gear_table(0.1);  // this one too
  unsigned short expected[] = {500, 1500, 2500, 3500, 4500, 5500};
  compare_to_expected(expected, 6);
}

int main(void) {
  printf("Starting tests...\n\n");
  byte new_gear = 255;
  test1();
  test2();
  test3();
  test4();
  printf("\ndone.\n");
}
