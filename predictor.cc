/* Author: Mark Faust;
 * Edited by Ebele Esimai
 * Description: This file defines the two required functions for the branch predictor.
*/
#include <stdlib.h>
#include "predictor.h"

// number of bits
uint lbits = 10;
uint lpbits = 3;
uint gpbits = 2;
uint gbits = 12;

// masks
uint lmask = (1 << lbits) - 1;  // local index pc[11:2]
uint pmask = (1 << gbits) - 1;  // global history

// data structures
uint local_hist[1024] ={0};       // local history table
uint local_predt[1024] = {0};      // local prediction table
uint global_predt[4096] = {0};     // global prediction table
uint choice_predt[4096] = {0};     // choice prediction table
uint path_hist = 0;     // global path history

bool PREDICTOR::get_prediction(const branch_record_c *br, const op_state_c *os) {
    if (br->is_conditional){
        uint gindex = path_hist & pmask;
        uint choice = choice_predt[gindex] >> (gpbits - 1);
        if (choice == 0) {
            // use local predictor
            uint pindex = (br->instruction_addr >> 2) & lmask;
            uint lindex = local_hist[pindex];
            uint lpred = local_predt[lindex] >> (lpbits - 1);
            return (lpred == 1);
        }
        else{
            // use global predictor
            uint gpred = global_predt[gindex] >> (gpbits - 1);
            return (gpred == 1);
        }
    }
    return true;

}


// Update the predictor after a prediction has been made.  This should accept
// the branch record (br) and architectural state (os), as well as a third
// argument (taken) indicating whether or not the branch was taken.
void PREDICTOR::update_predictor(const branch_record_c *br, const op_state_c *os, bool taken) {
    int choice = (taken) ? 1 : 0;
    if (br->is_conditional){
        uint pindex = (br->instruction_addr >> 2) & lmask;
        uint lindex = local_hist[pindex];
        int lpred =  local_predt[lindex] >> (lpbits - 1);
        uint gindex = path_hist & pmask;
        int gpred = global_predt[gindex] >> (gpbits - 1);
        int lpgp = choice_predt[gindex];

        // update the choice prediction table
        if (lpred != gpred){
            if (lpred == choice)
                lpgp = std::max(lpgp - 1, 0);
            else
                lpgp = std::min(lpgp + 1, (1 << gpbits) - 1);
        }
        choice_predt[gindex] = (uint) lpgp;

        // update the local and global prediction table
        if (taken){
            lpred = local_predt[lindex] + 1;
            local_predt[lindex] = (uint) std::min(lpred, (1 << lpbits) - 1);
            gpred = global_predt[gindex] + 1;
            global_predt[gindex] = (uint) std::min(gpred, (1 << gpbits) - 1);
        }
        else{
            lpred = local_predt[lindex] - 1;
            local_predt[lindex] = (uint) std::max(lpred, 0);
            gpred = global_predt[gindex] - 1;
            global_predt[gindex] = (uint) std::max(gpred, 0);
        }

        // update the local history table
        local_hist[pindex] = (uint) ((lindex << 1) | choice) & lmask;

    }
    // update the path history table
    path_hist = (uint) ((path_hist << 1) | choice) & pmask;
}
