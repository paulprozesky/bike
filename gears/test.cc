#include <stdio.h>
#include <string.h>
#include "test.h"

const short RATIO_TOLERANCE = 200;  // absolute - unitless

byte MAX_GEARS = 9;

// store the ratios multiplied by 1000 so we don't need floats
unsigned short ratios[9] = {65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535};
byte num_ratios = 0;

void save_to_eeprom() {
  /* Save the ratio table in RAM to EEPROM
  */
}

byte search_array(unsigned short new_ratio) {
  /* It wasn't the first one and it wasn't bigger than the biggest, so where is it?
  */
  // find the first time it's smaller than a ratio's upper limit
  byte lower_match = MAX_GEARS;
  // loop through the ratios we know about and see where this new one fits in
  for (byte ctr = 0; ctr < num_ratios; ctr++) {
    short this_ratio = ratios[ctr];
    unsigned short this_ratio_upper_limit = this_ratio + RATIO_TOLERANCE;
    unsigned short this_ratio_lower_limit = this_ratio - RATIO_TOLERANCE;
    // find it a new home
    if ((new_ratio > this_ratio_lower_limit) && (new_ratio <= this_ratio_upper_limit))
      return MAX_GEARS;  // it matches this one, do nothing
    else if (new_ratio < this_ratio_lower_limit) {
      lower_match = ctr;  // it's smaller than this one
      break;
    }
    else if (new_ratio > this_ratio_upper_limit) {
      // it's bigger than this one, so carry on looking
    }
  }
  if (MAX_GEARS == lower_match) {
    // this should never happen
    printf("=======================================================\n");
    printf("ERROR: could not find a place for new ratio: %i\n", new_ratio);
    printf("=======================================================\n");
    return MAX_GEARS;
  }
  return lower_match;
}

void insert_new_ratio(byte new_gear, unsigned short new_ratio) {
  /* Given a new ratio with a new position, but it in the array in RAM
  */
  if (MAX_GEARS <= new_gear)
    return;
  unsigned short old_ratios[MAX_GEARS];
  memcpy(old_ratios, ratios, MAX_GEARS*sizeof(ratios[0]));
  // lower chunk has already been copied above
  // move the upper chunk up to make room for the new value
  for(byte ctr = new_gear + 1; ctr < MAX_GEARS; ctr++)
    ratios[ctr] = old_ratios[ctr - 1];
  ratios[new_gear] = new_ratio;
  num_ratios++;
  save_to_eeprom();
}

byte update_gear_table(float ratio) {
  /* Given a new ratio, find out where it should be in the list.
  */
  // do we have all of them already?
  if (num_ratios >= MAX_GEARS)
    return MAX_GEARS;
  // otherwise find out where to put this one
  unsigned short new_ratio = ratio * 1000;
  byte new_gear = MAX_GEARS;
  if (0 == num_ratios)
    new_gear = 0;  // it's the first one
  else if (new_ratio > (ratios[num_ratios - 1] + RATIO_TOLERANCE))
    new_gear = num_ratios;  // it's bigger than the current biggest one
  else
    new_gear = search_array(new_ratio);  // it's somewhere else
  // update the ratio table as required
  if (new_gear < MAX_GEARS)
    insert_new_ratio(new_gear, new_ratio);
  return new_gear;
}

/****************************************************************************************************
  TEST CODE BELOW
****************************************************************************************************/

void print_ratios(void) {
  /* Print the ratios in RAM
  */
  printf("ratios[");
  for(int ctr=0; ctr < 9; ctr++)
    printf("%i, ", ratios[ctr]);
  printf("]\n");
}

void reset_ratios(void) {
  /* Reset all the ratios currently stored in RAM.
  Does NOT change EEPROM.
  */
  for (byte ctr = 0; ctr < MAX_GEARS; ctr++)
    ratios[ctr] = 65535;
  num_ratios = 0;
}

void compare_to_expected(unsigned short *expected_ratios, byte num_expected) {
  /* Compare the ratios in RAM to a list of expected ratios.
  */
  byte errors = 0;
  for (byte ctr = 0; ctr < num_expected; ctr++) {
    if (ratios[ctr] != expected_ratios[ctr])
      errors++;
  }
  if (errors > 0) {
    printf("ERRORS:\n");
    print_ratios();
  }
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
  update_gear_table(7.5);
  update_gear_table(0.1);
  unsigned short expected[] = {100, 500, 1500, 2500, 3500, 4500, 5500, 7500};
  compare_to_expected(expected, 8);
}

int main(void) {
  /* Run a couple of tests to exercise the search functions.
  */
  printf("Starting tests...\n\n");
  test1();
  test2();
  test3();
  test4();
  printf("\ndone.\n");
}
