//
//  sd_ios_time_wrapper.m
//  common
//
//  Created by dragon on 11/12/10.
//  Copyright 2010 Thunder Networking Inc. All rights reserved.
//

#import "sd_ios_time_wrapper.h"
#import <Foundation/NSDate.h>
#import <Foundation/NSDateFormatter.h>

#if defined(MACOS) && defined(MOBILE_PHONE)

_int32 sd_ios_time_time(_u32 *times)
{
	return 0;
}

_int32 sd_ios_time_time_ms(_u32 *time_ms)
{
	return 0;
}

_int32 sd_ios_time_get_current_time_info(_int64 *time_stamp, _u32 *year, _u32 *mon, _u32 *day, _u32 *hour, _u32 *min, _u32 *sec, _u32 *msec,_u32 *day_year, _u32 *day_week, _u32 *week_year)
{
      NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSDate *today = [NSDate date];
	NSDateFormatter *date_formater=[[NSDateFormatter alloc]init];
	
	[date_formater setDateFormat:@"YYYY"];
	*year = [[date_formater stringFromDate:today] intValue];        
	
	[date_formater setDateFormat:@"MM"];
	*mon = [[date_formater stringFromDate:today] intValue];        
	
	[date_formater setDateFormat:@"dd"];
	*day = [[date_formater stringFromDate:today] intValue];	

	[date_formater setDateFormat:@"HH"];
	*hour = [[date_formater stringFromDate:today] intValue];        
	
	[date_formater setDateFormat:@"mm"];
	*min = [[date_formater stringFromDate:today] intValue];        
	
	[date_formater setDateFormat:@"ss"];
	*sec = [[date_formater stringFromDate:today] intValue];
	
	[date_formater setDateFormat:@"SSS"];
	*msec = [[date_formater stringFromDate:today] intValue];
	
	[date_formater release];
	[pool drain];
	return 0;
}

_int32 sd_ios_time_get_time_info(_u32 times, _int64 *time_stamp,_u32 *year, _u32 *mon, _u32 *day, _u32 *hour, _u32 *min, _u32 *sec, _u32 *msec, _u32 *day_year, _u32 *day_week, _u32 *week_year)
{
//	- (int)yearOfCommonEra;
//	- (int)monthOfYear;
//	- (int)dayOfMonth;
//	- (int)dayOfWeek;
//	- (int)dayOfYear;
//	- (int)dayOfCommonEra;
//	- (int)hourOfDay;
//	- (int)minuteOfHour;
//	- (int)secondOfMinute;


	return 0;
}

#endif 