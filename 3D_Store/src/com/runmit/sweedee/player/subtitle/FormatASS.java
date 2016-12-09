package com.runmit.sweedee.player.subtitle;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

/**
 * Class that represents the .ASS and .SSA subtitle file format
 * <p/>
 * <br><br>
 * Copyright (c) 2012 J. David Requejo <br>
 * j[dot]david[dot]requejo[at] Gmail
 * <br><br>
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 * <br><br>
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 * <br><br>
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * @author J. David REQUEJO
 */
public class FormatASS implements TimedTextFileFormat {



	/* PRIVATEMETHODS */

    /**
     * This methods transforms a format line from ASS according to a format definition into an Style object.
     *
     * @param line the format line without its declaration
     * @param styleFormat the list of attributes in this format line
     * @return a new Style object.
     */
//	private Style parseStyleForASS(String[] line, String[] styleFormat, int index, boolean isASS, String warnings) {
//
//		Style newStyle = new Style(Style.defaultID());
//		if (line.length != styleFormat.length){
//			//both should have the same size
//			warnings+="incorrectly formated line at "+index+"\n\n";
//		} else {
//			for (int i = 0; i < styleFormat.length; i++) {
//				//we go through every format parameter and save the interesting values
//				if (styleFormat[i].trim().equalsIgnoreCase("Name")){
//					//we save the name
//					newStyle.iD=line[i].trim();
//				} else if (styleFormat[i].trim().equalsIgnoreCase("Fontname")){
//					//we save the font
//					newStyle.font=line[i].trim();
//				} else if (styleFormat[i].trim().equalsIgnoreCase("Fontsize")){
//					//we save the size
//					newStyle.fontSize=line[i].trim();
//				}else if (styleFormat[i].trim().equalsIgnoreCase("PrimaryColour")){
//					//we save the color
//					String color =line[i].trim();
//					if(isASS){
//						if(color.startsWith("&H")) newStyle.color=Style.getRGBValue("&HAABBGGRR", color);
//						else  newStyle.color=Style.getRGBValue("decimalCodedAABBGGRR", color);
//					} else {
//						if(color.startsWith("&H")) newStyle.color=Style.getRGBValue("&HBBGGRR", color);
//						else  newStyle.color=Style.getRGBValue("decimalCodedBBGGRR", color);
//					}
//				}else if (styleFormat[i].trim().equalsIgnoreCase("BackColour")){
//					//we save the background color
//					String color =line[i].trim();
//					if(isASS){
//						if(color.startsWith("&H")) newStyle.backgroundColor=Style.getRGBValue("&HAABBGGRR", color);
//						else  newStyle.backgroundColor=Style.getRGBValue("decimalCodedAABBGGRR", color);
//					} else {
//						if(color.startsWith("&H")) newStyle.backgroundColor=Style.getRGBValue("&HBBGGRR", color);
//						else  newStyle.backgroundColor=Style.getRGBValue("decimalCodedBBGGRR", color);
//					}
//				}else if (styleFormat[i].trim().equalsIgnoreCase("Bold")){
//					//we save if bold
//					newStyle.bold=Boolean.parseBoolean(line[i].trim());
//				}else if (styleFormat[i].trim().equalsIgnoreCase("Italic")){
//					//we save if italic
//					newStyle.italic=Boolean.parseBoolean(line[i].trim());
//				}else if (styleFormat[i].trim().equalsIgnoreCase("Underline")){
//					//we save if underlined
//					newStyle.underline=Boolean.parseBoolean(line[i].trim());
//				}else if (styleFormat[i].trim().equalsIgnoreCase("Alignment")){
//					//we save the alignment
//					int placement =Integer.parseInt(line[i].trim());
//					if (isASS){
//						switch(placement){
//						case 1:
//							newStyle.textAlign="bottom-left";
//							break;
//						case 2:
//							newStyle.textAlign="bottom-center";
//							break;
//						case 3:
//							newStyle.textAlign="bottom-right";
//							break;
//						case 4:
//							newStyle.textAlign="mid-left";
//							break;
//						case 5:
//							newStyle.textAlign="mid-center";
//							break;
//						case 6:
//							newStyle.textAlign="mid-right";
//							break;
//						case 7:
//							newStyle.textAlign="top-left";
//							break;
//						case 8:
//							newStyle.textAlign="top-center"; 
//							break;
//						case 9:
//							newStyle.textAlign="top-right";
//							break;
//						default:
//							warnings+="undefined alignment for style at line "+index+"\n\n";
//						}
//					} else {
//						switch(placement){
//						case 9:
//							newStyle.textAlign="bottom-left";
//							break;
//						case 10:
//							newStyle.textAlign="bottom-center";
//							break;
//						case 11:
//							newStyle.textAlign="bottom-right";
//							break;
//						case 1:
//							newStyle.textAlign="mid-left";
//							break;
//						case 2:
//							newStyle.textAlign="mid-center";
//							break;
//						case 3:
//							newStyle.textAlign="mid-right";
//							break;
//						case 5:
//							newStyle.textAlign="top-left";
//							break;
//						case 6:
//							newStyle.textAlign="top-center"; 
//							break;
//						case 7:
//							newStyle.textAlign="top-right";
//							break;
//						default:
//							warnings+="undefined alignment for style at line "+index+"\n\n";
//						}
//					}
//				}	
//				
//			}
//		}
//		
//		return newStyle;
//	}

    /**
     * This methods transforms a dialogue line from ASS according to a format definition into an Caption object.
     *
     * @param line           the dialogue line without its declaration
     * @param dialogueFormat the list of attributes in this dialogue line
     * @param timer          % to speed or slow the clock, above 100% span of the subtitles is reduced.
     * @return a new Caption object
     */
    private Caption parseDialogueForASS(String[] line, String[] dialogueFormat, float timer) {

        Caption newCaption = new Caption();

        //all information from fields 10 onwards are the caption text therefore needn't be split
        String captionText = line[9];
        //text is cleaned before being inserted into the caption
        newCaption.content = captionText.replaceAll("\\{.*?\\}", "").replace("\n", "<br />").replace("\\N", "<br />");

        for (int i = 0; i < dialogueFormat.length; i++) {
            //we go through every format parameter and save the interesting values
            Log.d("tag", "dialogueFormat=" + dialogueFormat[i].trim());
            if (dialogueFormat[i].trim().equalsIgnoreCase("Style")) {
                //we save the style
//				Style s =  tto.styling.get(line[i].trim());
//				if (s!=null)
//					newCaption.style= s;
//				else
//					tto.warnings+="undefined style: "+line[i].trim()+"\n\n";
            } else if (dialogueFormat[i].trim().equalsIgnoreCase("Start")) {
                //we save the starting time
                Log.d("load", "start line[i]=" + line[i]);
                newCaption.start = new Time("h:mm:ss.ms", line[i].trim());
            } else if (dialogueFormat[i].trim().equalsIgnoreCase("End")) {
                //we save the starting time
                Log.d("load", "end line[i]=" + line[i]);
                newCaption.end = new Time("h:mm:ss.ms", line[i].trim());
            }
        }

        //timer is applied
        if (timer != 100) {
            newCaption.start.mseconds /= (timer / 100);
            newCaption.end.mseconds /= (timer / 100);
        }
        return newCaption;
    }

//	private Caption parseDialogueForASS(String[] line, String[] dialogueFormat, float timer) {
//		
//		Caption newCaption = new Caption();
//		
//		//all information from fields 10 onwards are the caption text therefore needn't be split
//		String captionText = line[9];
//		//text is cleaned before being inserted into the caption
//		newCaption.content = captionText.replaceAll("\\{.*?\\}", "").replace("\n", "<br />").replace("\\N", "<br />");		
//		
//		for (int i = 0; i < dialogueFormat.length; i++) {
//			//we go through every format parameter and save the interesting values
//			if (dialogueFormat[i].trim().equalsIgnoreCase("Style")){
//			} else if (dialogueFormat[i].trim().equalsIgnoreCase("Start")){
//				//we save the starting time
//				newCaption.start=new Time("h:mm:ss.cs",line[i].trim());
//			} else if (dialogueFormat[i].trim().equalsIgnoreCase("End")){
//				//we save the starting time
//				newCaption.end=new Time("h:mm:ss.cs",line[i].trim());
//			}
//		}
//		
//		//timer is applied
//    	if (timer != 100){
//    		newCaption.start.mseconds /= (timer/100);
//    		newCaption.end.mseconds /= (timer/100);
//    	}
//		return newCaption;
//	}

    /**
     * returns a string with the correctly formated colors
     * @param useASSInsteadOfSSA true if formated for ASS
     * @return the colors in the decimal format
     */
//	private String getColorsForASS(boolean useASSInsteadOfSSA, Style style) {
//		String colors;
//		if(useASSInsteadOfSSA)
//			//primary color(BBGGRR) with Alpha level (00) in front + 00FFFFFF + 00000000 + background color(BBGGRR) with Alpha level (80) in front
//			colors=Integer.parseInt("00"+ style.color.substring(4,6)+style.color.substring(2, 4)+style.color.substring(0, 2), 16)+",16777215,0,"+Long.parseLong("80"+ style.backgroundColor.substring(4,6)+style.backgroundColor.substring(2, 4)+style.backgroundColor.substring(0, 2), 16)+",";
//		else {
//			//primary color(BBGGRR) + FFFFFF + 000000 + background color(BBGGRR)
//			String color = style.color.substring(4,6)+style.color.substring(2, 4)+style.color.substring(0, 2);
//			String bgcolor = style.backgroundColor.substring(4,6)+style.backgroundColor.substring(2, 4)+style.backgroundColor.substring(0, 2);
//			colors=Long.parseLong(color, 16)+",16777215,0,"+Long.parseLong(bgcolor, 16)+",";	
//		}
//		return colors;
//	}

    /**
     * returns a string with the correctly formated options
     * @param useASSInsteadOfSSA
     * @return
     */
//	private String getOptionsForASS(boolean useASSInsteadOfSSA, Style style) {
//		String options;
//		if (style.bold)
//			options="-1,";
//		else
//			options="0,";
//		if (style.italic)
//			options+="-1,";
//		else
//			options+="0,";
//		if(useASSInsteadOfSSA){
//			if (style.underline)
//				options+="-1,";
//			else
//				options+="0,";
//			options+="0,100,100,0,0,";
//		}	
//		return options;
//	}

    /**
     * converts the string explaining the alignment into the ASS equivalent integer offering bottom-center as default value
     *
     * @param useASSInsteadOfSSA
     * @param align
     * @return
     */
//	private int getAlignForASS(boolean useASSInsteadOfSSA, String align) {
//		if (useASSInsteadOfSSA){
//			int placement = 2;
//			if ("bottom-left".equals(align))
//				placement = 1;
//			else if ("bottom-center".equals(align))
//				placement = 2;
//			else if ("bottom-right".equals(align))
//				placement = 3;
//			else if ("mid-left".equals(align))
//				placement = 4;
//			else if ("mid-center".equals(align))
//				placement = 5;
//			else if ("mid-right".equals(align))
//				placement = 6;
//			else if ("top-left".equals(align))
//				placement = 7;
//			else if ("top-center".equals(align))
//				placement = 8;
//			else if ("top-right".equals(align))
//				placement = 9;
//			
//			return placement;
//		} else {
//
//			int placement = 10;
//			if ("bottom-left".equals(align))
//				placement = 9;
//			else if ("bottom-center".equals(align))
//				placement = 10;
//			else if ("bottom-right".equals(align))
//				placement = 11;
//			else if ("mid-left".equals(align))
//				placement = 1;
//			else if ("mid-center".equals(align))
//				placement = 2;
//			else if ("mid-right".equals(align))
//				placement = 3;
//			else if ("top-left".equals(align))
//				placement = 5;
//			else if ("top-center".equals(align))
//				placement = 6;
//			else if ("top-right".equals(align))
//				placement = 7;
//			
//			return placement;
//		}
//	}
    public boolean loadFileToDB(String fileName, String charsetName,
                                Context context) {
        InputStream is;
        try {
            is = new FileInputStream(new File(fileName));
        } catch (FileNotFoundException e1) {
            e1.printStackTrace();
            return false;
        }
        InputStreamReader in = null;
        try {
            in = new InputStreamReader(is, charsetName);
        } catch (UnsupportedEncodingException e1) {
            e1.printStackTrace();
            return false;
        }
        Pattern pa_real = Pattern.compile("Dialogue:[\\s]*[\\d]+,(\\d:\\d{2}:\\d{2}.\\d{2},\\d:\\d{2}:\\d{2}.\\d{2})");
        Pattern pa_other = Pattern.compile("\\d:\\d{2}:\\d{2}.\\d{2}[\\s]*-->[\\s]*\\d:\\d{2}:\\d{2}.\\d{2}");
        BufferedReader br = new BufferedReader(in);
        String str = null;
        while ((str = readLine(br)) != null) {
            Matcher ma = pa_real.matcher(str);
            if (ma.find()) {
                try {
                    is.close();
                    in.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
                return loadFileToDBForReal(fileName, charsetName, context);
            }

            ma = pa_other.matcher(str);
            if (ma.find()) {
                try {
                    is.close();
                    in.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
                return loadFileToDBForOther(fileName, charsetName, context);
            }
        }
        return false;
    }


    public boolean loadFileToDBForReal(String fileName, String charsetName,
                                       Context context) {
        File file = new File(fileName);
        InputStream is;
        try {
            is = new FileInputStream(file);
        } catch (FileNotFoundException e1) {
            e1.printStackTrace();
            return false;
        }
        SubTilteDatabaseHelper mSubTilteDatabaseHelper = SubTilteDatabaseHelper.getInstance(context, fileName + ".db");
        Caption caption = new Caption();
//		Style style;

        //for the clock timer
        float timer = 100;

        //if the file is .SSA or .ASS
        boolean isASS = false;

        //variables to store the formats
        String[] styleFormat;
        String[] dialogueFormat;

        //first lets load the file
        InputStreamReader in = null;
        try {
            in = new InputStreamReader(is, charsetName);
        } catch (UnsupportedEncodingException e1) {
            // TODO Auto-generated catch block
            e1.printStackTrace();
            return false;
        }
        BufferedReader br = new BufferedReader(in);

        String line = null;
        int lineCounter = 0;
        try {
            //we scour the file
            line = readLine(br);
            lineCounter++;
            while (line != null) {

                //we skip any line until we find a section [section name]
                if (line.startsWith("[")) {
                    //now we must identify the section
                    if (line.equalsIgnoreCase("[Script info]")) {
                        //its the script info section section
                        lineCounter++;
                        line = readLine(br);
                        //Each line is scanned for useful info until a new section is detected
                        while (!line.startsWith("[")) {
//							if(line.startsWith("Title:"))
//								//We have found the title
//								tto.title = line.split(":")[1].trim();
//							else if (line.startsWith("Original Script:"))
//								//We have found the author
//								tto.author = line.split(":")[1].trim();
//							else if (line.startsWith("Script Type:")){
//								//we have found the version
//								if(line.split(":")[1].trim().equalsIgnoreCase("v4.00+"))isASS = true;
//								//we check the type to set isASS or to warn if it comes from an older version than the studied specs
//								else if(!line.split(":")[1].trim().equalsIgnoreCase("v4.00"))
//									tto.warnings+="Script version is older than 4.00, it may produce parsing errors.";
//							} else if (line.startsWith("Timer:"))
//								//We have found the timer
//								timer = Float.parseFloat(line.split(":")[1].trim().replace(',','.'));
                            //we go to the next line
                            lineCounter++;
                            line = readLine(br);
                        }

                    } else if (line.equalsIgnoreCase("[v4 Styles]")
                            || line.equalsIgnoreCase("[v4 Styles+]")
                            || line.equalsIgnoreCase("[v4+ Styles]")) {
                        //its the Styles description section
                        if (line.contains("+") && isASS == false) {
                            //its ASS and it had not been noted
                            isASS = true;
//							tto.warnings+="ScriptType should be set to v4:00+ in the [Script Info] section.\n\n";
                        }
                        lineCounter++;
                        line = readLine(br);
                        //the first line should define the format
                        if (!line.startsWith("Format:")) {
                            //if not, we scan for the format.
//							tto.warnings+="Format: (format definition) expected at line "+line+" for the styles section\n\n";
                            while (!line.startsWith("Format:")) {
                                lineCounter++;
                                line = readLine(br);
                            }
                        }
                        // we recover the format's fields
                        styleFormat = line.split(":")[1].trim().split(",");
                        lineCounter++;
                        line = readLine(br);
                        // we parse each style until we reach a new section
                        while (!line.startsWith("[")) {
                            //we check it is a style
                            if (line.startsWith("Style:")) {
                                //we parse the style
//								style = parseStyleForASS(line.split(":")[1].trim().split(","),styleFormat,lineCounter,isASS,"");
                                //and save the style
//								tto.styling.put(style.iD, style);
                            }
                            //next line
                            lineCounter++;
                            line = readLine(br);
                        }

                    } else if (line.trim().equalsIgnoreCase("[Events]")) {
                        //its the events specification section
                        lineCounter++;
                        line = readLine(br);
//						tto.warnings+="Only dialogue events are considered, all other events are ignored.\n\n";
                        //the first line should define the format of the dialogues
                        if (!line.startsWith("Format:")) {
                            //if not, we scan for the format.
//							tto.warnings+="Format: (format definition) expected at line "+line+" for the events section\n\n";
                            while (!line.startsWith("Format:")) {
                                lineCounter++;
                                line = readLine(br);
                            }
                        }
                        // we recover the format's fields
                        dialogueFormat = line.split(":")[1].trim().split(",");
                        //next line
                        lineCounter++;
                        line = readLine(br);
                        // we parse each style until we reach a new section
                        while (!line.startsWith("[")) {
                            //we check it is a dialogue
                            //WARNING: all other events are ignored.
                            if (line.startsWith("Dialogue:")) {
                                //we parse the dialogue
                                caption = parseDialogueForASS(line.split(":", 2)[1].trim().split(",", 10), dialogueFormat, timer);
                                //and save the caption
                                int key = caption.start.mseconds;
                                //in case the key is already there, we increase it by a millisecond, since no duplicates are allowed
//								while (tto.captions.containsKey(key)) key++;
                                Log.d("tag", "caption=" + caption.toString());
                                mSubTilteDatabaseHelper.addCaption(caption.toContentValues());
                            }
                            //next line
                            lineCounter++;
                            line = readLine(br);
                        }

                    } else if (line.trim().equalsIgnoreCase("[Fonts]") || line.trim().equalsIgnoreCase("[Graphics]")) {
                        //its the custom fonts or embedded graphics section
                        //these are not supported
//						tto.warnings+= "The section "+line.trim()+" is not supported for conversion, all information there will be lost.\n\n";
                        line = readLine(br);
                    } else {
//						tto.warnings+= "Unrecognized section: "+line.trim()+" all information there is ignored.";
                        line = readLine(br);
                    }
                } else {
                    line = readLine(br);
                    lineCounter++;
                }
            }
            // parsed styles that are not used should be eliminated
//			tto.cleanUnusedStyles();

        } catch (NullPointerException e) {
//			tto.warnings+= "unexpected end of file, maybe last caption is not complete.\n\n";
        } finally {
            try {
                is.close();
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
        mSubTilteDatabaseHelper.setVersion(SubTilteDatabaseHelper.VERSION_LOADED);
//		tto.built = true;
        return true;
    }


    private String readLine(BufferedReader br) {
        String s = null;
        try {
            s = br.readLine();
            if (s != null) {
                return s.trim();
            }
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return s;
    }


    public boolean loadFileToDBForOther(String fileName, String charsetName, Context context) {
        File file = new File(fileName);
        InputStream is = null;
        try {
            is = new FileInputStream(file);
        } catch (FileNotFoundException e1) {
            e1.printStackTrace();
        }

        Caption caption = new Caption();
        // first lets load the file
        InputStreamReader in = null;
        SubTilteDatabaseHelper mSubTilteDatabaseHelper = SubTilteDatabaseHelper.getInstance(context, fileName + ".db");
        SQLiteDatabase db = mSubTilteDatabaseHelper.getWritableDatabase();
        try {
            in = new InputStreamReader(is, charsetName);
        } catch (UnsupportedEncodingException e1) {
            e1.printStackTrace();
        }

        BufferedReader br = new BufferedReader(in);
        String line = null;

        try {
            line = br.readLine().trim();
            while (line != null) {
                line = line.trim();
                // if its a blank line, ignore it, otherwise...
                if (line.length() > 0) {
                    final String timeline = br.readLine().trim();
                    final String contentline = br.readLine().trim();

                    String start = timeline.substring(0, 10);
                    String end = timeline.substring(timeline.length() - 10, timeline.length());
                    Time time = new Time("h:mm:ss.ms", start);
                    caption.start = time;
                    time = new Time("h:mm:ss.ms", end);
                    caption.end = time;

                    String text = "";
                    line = contentline;
                    while (line.length() > 0) {
                        text += line + "<br />";
                        line = br.readLine();
                        if (line == null) {
                            break;
                        }
                        line = line.trim();
                    }
                    caption.content = text.replace("\\N", "<br />");
                    db.insert(Caption.TABLE_NAME, null, caption.toContentValues());
                    // we go to next blank
                    while (line != null && line.length() > 0) {    //不判断空会有空指针，解决bug 107671
                        line = br.readLine().trim();
                    }
                    caption = new Caption();
                }
                line = br.readLine();

            }
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        } finally {
            try {
                is.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        db.close();
        mSubTilteDatabaseHelper.setVersion(SubTilteDatabaseHelper.VERSION_LOADED);
        Log.d("mSubTilteDatabaseHelper", "mSubTilteDatabaseHelper=" + mSubTilteDatabaseHelper.getVersion());
        return true;
    }
}
