# Open-Source-Text-Reader
My Contribution to an Open Source Text Reader

Uses the hardware and start up instruction found in both of these repositories:
Look for the setup guide in:
https://github.com/joeycastillo/The-Open-Book
https://github.com/joeycastillo/babel

I have a few populated pcb boards available. Very few.

I am thinking about how to incorporate an html parser for the reader, how can pictures be linked to when viewing, among other things. The trouble is, every time a link to a picture is selected, a refresh would need to be done on the screen. I think that would be annoying.

I am not a software engineer. My experience is in industrial automation controls, mostly in robotics. This is my first programming of this type of application, though I have incorporated arduinos into some other projects before. I am open to suggestions on how to make the coding better, bug fixes, and the like.

All that is needed is to add txt files into the sd card. It generates the opnbk.csv file with a list of the text files, and some place holders. I mostly use the select, lock, up and down, and forward and backward buttons. If you are reading, lock will hold your place, and show that you are continuing reading. Select will "closed" the opnbk.csv entry. Both will hold your place. They are unsigned longs. Hopefully your txt file is less than 4,294,967,295 characters in length.

While the 3 color LED at the bottom is bright, things are happening in the background to make the buttoms unresponsive. Wait till the 3 color LED is a paler version of what it was.

I am still debugging why some lines are short, when the next character should be on the same line, or why some lines run long. It is probably something simple. We will see.


Twitter: @AndrewVall to get in touch with me.
