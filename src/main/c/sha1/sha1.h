// Copyright (c) 2013-2017, Matt Godbolt
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// DO NOT REFORMAT <- for tidy
/*
 *  sha1.h
 *
 *  Copyright (C) 1998, 2009
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved.
 *
 *****************************************************************************
 *  $Id: sha1.h 12 2009-06-22 19:34:25Z paulej $
 *****************************************************************************
 *
 *  Description:
 *      This class implements the Secure Hashing Standard as defined
 *      in FIPS PUB 180-1 published April 17, 1995.
 *
 *      Many of the variable names in this class, especially the single
 *      character names, were used because those were the names used
 *      in the publication.
 *
 *      Please read the file sha1.cpp for more information.
 *
 */

#ifndef _SHA1_H_
#define _SHA1_H_

class SHA1 {

public:
    SHA1();

    /*
         *  Re-initialize the class
         */
    void Reset();

    /*
         *  Returns the message digest
         */
    bool Result(unsigned* message_digest_array);

    /*
         *  Provide input to SHA1
         */
    void Input(const unsigned char* message_array,
               unsigned length);
    void Input(const char* message_array,
               unsigned length);
    void Input(unsigned char message_element);
    void Input(char message_element);
    SHA1& operator<<(const char* message_array);
    SHA1& operator<<(const unsigned char* message_array);
    SHA1& operator<<(const char message_element);
    SHA1& operator<<(const unsigned char message_element);

private:
    /*
         *  Process the next 512 bits of the message
         */
    void ProcessMessageBlock();

    /*
         *  Pads the current message block to 512 bits
         */
    void PadMessage();

    /*
         *  Performs a circular left shift operation
         */
    inline unsigned CircularShift(int bits, unsigned word);

    unsigned H[5]; // Message digest buffers

    unsigned Length_Low;  // Message length in bits
    unsigned Length_High; // Message length in bits

    unsigned char Message_Block[64]; // 512-bit message blocks
    int Message_Block_Index;         // Index into message block array

    bool Computed;  // Is the digest computed?
    bool Corrupted; // Is the message digest corruped?
};
#endif
