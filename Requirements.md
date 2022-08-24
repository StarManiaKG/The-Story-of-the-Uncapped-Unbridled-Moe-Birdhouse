# Requirements #

I'm not the only person making decisions on this front -- i'd say a change this
big would probably have to be run past our actual menu designers -- but what it
would personally take for me to approve such a merge request:

1) no mixels, so don't use downscaled text or other graphics (the only
   exception is the main player sprite for hires chars, but please don't make it
   any smaller than it currently is). only unflexible ironclad rule, everything
   else open to discussion
2) some attention to visual design, so it's not just a solid wall of characters
   with no aesthetic spacing, and the relevant labels for things are in logical
   locations. i completely understand if you don't have that kind of experience
   and don't expect the mona lisa, but something that's raw practicality with no
   attention to readability is a no go for how much attention's gone into the
   rest of the game's UI.
3) the default mode shouldn't de-emphasise the stat grid, which requires its
   dividing lines that split it into the different engine classes. it doesn't
   strictly need any of the other labels, but considering previous v1 patches
   ADDED that that probably makes sense to keep
4) this is an area Ashnal has rightfully identified as a conflict of interest
   between the Uncle MUGEN mindset and Krew, but we'd prefer the default five
   characters not buried. This discourages automatic cycling on the statgrid
   option unless you're literally hovering your cursor over it

One possible way of satisfying these could be to have the menu be basically
identical in normal function, but display a darkening overlay when hitting the
"Enter" key (or equivalent) while the Character option is selected that shows a
grid on top. "Press enter for Extended View", or something. But that's not the
only viable approach, and open to other exploration

Also, if you need to adjust the spacing as part of actual design attention, I
suggest cutting the height of the color bar in half. Use Tails' sprites to
figure out which indices to keep.

oh yeah, needs to be usable at 320*200 -- this falls under no mixels but i'm
literally at the discord message length limit lmao
