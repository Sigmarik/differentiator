/**
 * @file common_flags.h
 * @author Kudryashov Ilya (kudriashov.it@phystech.edu)
 * @brief Flags present in main program.
 * @version 0.1
 * @date 2022-10-23
 * 
 * @copyright Copyright (c) 2022
 * 
 */

{ {'O', "owl"}, { {}, 0, print_owl },
    "print 10 owls to the screen." },

{ {'I', ""}, { log_threshold_wrapper, 1, edit_int },
    "set log threshold to the specified number.\n"
    "\tDoes not check if integer was specified." },

{ {'S', "silent"}, { {}, 0, mute_speaker } }
