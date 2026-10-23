#!/usr/bin/env python3
"""Rebuild words.txt: the 500 most common English words of 3+ letters.

The ranking is wordfreq's (pip install wordfreq), which blends Wikipedia,
subtitles, news, books and web text. Words with anything but a-z, words
under 3 letters, names, slang and swearing are left out.

Usage: python3 make_words.py > words.txt
"""
import re
import wordfreq

SKIP = set("""
shit fuck fucking sex gonna yeah non per
john york london america american british english april june july march
""".split())

words = [w for w in wordfreq.top_n_list("en", 2000)
         if re.fullmatch("[a-z]{3,}", w) and w not in SKIP][:500]
print("# The 500 most common English words of 3+ letters (wordfreq ranking),")
print("# most common first. One word per line; edit freely, '#' starts a comment.")
print("\n".join(words))
