<?php
require_once('devtools.inc.php');
include 'common.inc';

/**
* Load the page results directly from the results files
* 
* @param mixed $id
* @param mixed $testPath
* @param mixed $run
* @param mixed $cached
*/
function loadAllPageData($testPath, $options = null, $multistep = false) {
  $ret = array();
  // go in order for the number of runs there are supposed to be
  if (is_file("$testPath/testinfo.ini")) {
    $ini = parse_ini_file("$testPath/testinfo.ini", true);
    $runs = $ini['test']['runs'];
    $fvonly = $ini['test']['fvonly'];
    $testInfo = GetTestInfo($testPath);
    $completed = true;
    if ($testInfo && (!array_key_exists('completed', $testInfo) || !$testInfo['completed']))
      $completed = false;

    for( $run = 1; $run <= $runs; $run++ ) {
      // only load page data for individual runs that are complete (or if the whole test is complete)
      if ($completed || !$testInfo || IsTestRunComplete($run, $testInfo)) {
        $data = loadPageRunData($testPath, $run, 0, $options, $multistep);
        if( isset($data) )
          $ret[$run][0] = $data;
        
        if( !$fvonly ) {
          unset( $data );
          $data = loadPageRunData($testPath, $run, 1, $options, $multistep);
          if( isset($data) )
            $ret[$run][1] = $data;
        }
      }
    }
  }
  logAlways("loadAllPageData: ret = " . json_encode($ret));
  return $ret;
}

/**
* Load the page data for the given run
* 
* @param mixed $testPath
* @param mixed $run
* @param mixed $fv
* @param mixed $rv
*/
function loadPageRunData($testPath, $run, $cached, $options = null, $multistep=false)
{
    $ret = null;
    $cachedText = $cached ? '_Cached' : '';
    $ret = loadPageData("$testPath/{$run}{$cachedText}_IEWPG.txt", $options, $multistep);
    if (!isset($ret) || !is_array($ret) || !count($ret))
      GetDevToolsRequests($testPath, $run, $cached, $requests, $ret);
    // see if we have video files to calculate a visually complete time from
    $basic_results = false;
    if (array_key_exists('basic', $_REQUEST) && $_REQUEST['basic'])
      $basic_results = true;
    if (!$multistep && gz_is_file("$testPath/{$run}{$cachedText}_page_data.json")) {
      // Not supported with multi-step.
      $pd = json_decode(gz_file_get_contents("$testPath/{$run}{$cachedText}_page_data.json"), true);
      if ($pd && is_array($pd) && count($pd)) {
        foreach($pd as $key => $value) {
          $ret[$key] = $value;
        }
      }
    }

    $pages = array(&$ret);
    if ($multistep) {
      $pages = array();
      foreach ($ret as &$perStepPageData) {
        $pages[] = &$perStepPageData;
      }
    }
    foreach ($pages as &$eachPage) {
      if (isset($eachPage) && !$basic_results) {
        $startOffset = array_key_exists('testStartOffset', $eachPage) ? intval(round($eachPage['testStartOffset'])) : 0;
        if (!$multistep) {
          // TODO (kk): should fix this.
          loadUserTimingData($eachPage, $testPath, $run, $cached);

          // see if we have custom metrics to load
          if (gz_is_file("$testPath/{$run}{$cachedText}_metrics.json")) {
            $custom_metrics = json_decode(gz_file_get_contents("$testPath/{$run}{$cachedText}_metrics.json"), true);
            if ($custom_metrics && is_array($custom_metrics) && count($custom_metrics)) {
              $eachPage["custom"] = array();
              foreach ($custom_metrics as $metric => $value) {
                if (preg_match('/^[0-9]+$/', $value))
                  $eachPage[$metric] = intval($value);
                elseif (preg_match('/^[0-9]*\.[0-9]+$/', $value))
                  $eachPage[$metric] = floatval($value);
                else
                  $eachPage[$metric] = $value;
                $eachPage["custom"][] = $metric;
              }
            }
          }

          // see if we have CSI metrics to load
          if (is_dir('./google') && is_file('./google/google_lib.inc')) {
            require_once('google/google_lib.inc');
            $csi = ParseCsiInfo(0, $testPath, $run, $cached, true, false);
            if (isset($csi) && is_array($csi) && count($csi)) {
              $eachPage['CSI'] = array();
              foreach ($csi as $metric => $value) {
                if (preg_match('/^[0-9]+$/', $value))
                  $value = intval($value);
                elseif (preg_match('/^[0-9\.]+$/', $value))
                  $value = floatval($value);
                $eachPage["CSI.$metric"] = $value;
                $eachPage['CSI'][] = $metric;
              }
            }
          }
        }

        if (array_key_exists('loadTime', $eachPage) &&
            !$eachPage['loadTime'] &&
            array_key_exists('fullyLoaded', $eachPage) &&
            $eachPage['fullyLoaded'] > 0
        )
          $eachPage['loadTime'] = $eachPage['fullyLoaded'];
        $video_dir = "$testPath/video_$run";
        if ($cached)
          $video_dir .= '_cached';
        if (is_dir($video_dir)) {
          $frames = null;
          loadVideo($video_dir, $frames);
          if (isset($frames) && is_array($frames) && count($frames)) {
            if (!array_key_exists('lastVisualChange', $eachPage) || !$eachPage['lastVisualChange']) {
              end($frames);
              $last = max(key($frames) - $startOffset, 0);
              reset($frames);
              if ($last) {
                $eachPage['lastVisualChange'] = $last;
                if (!array_key_exists('visualComplete', $eachPage))
                  $eachPage['visualComplete'] = $eachPage['lastVisualChange'];
              }
            }
            if ((!array_key_exists('render', $eachPage) || !$eachPage['render']) && count($frames) > 1) {
              next($frames);
              $first = max(key($frames) - $startOffset, 0);
              reset($frames);
              if ($first)
                $eachPage['render'] = $first;
            }
          }
        }
        // TODO (kk): should fix this.
        require_once('./video/visualProgress.inc.php');
        $progress = GetVisualProgress($testPath, $run, $cached, null, null, $startOffset);
        if (isset($progress) && is_array($progress)) {
          if (array_key_exists('SpeedIndex', $progress))
            $eachPage['SpeedIndex'] = $progress['SpeedIndex'];
          if (array_key_exists('visualComplete', $progress))
            $eachPage['visualComplete'] = $progress['visualComplete'];
          if (array_key_exists('startRender', $progress) && (!array_key_exists('render', $eachPage) || !$eachPage['render']))
            $eachPage['render'] = $progress['startRender'];
          if ((!array_key_exists('lastVisualChange', $eachPage) ||
                  !$eachPage['lastVisualChange']) &&
              array_key_exists('visualComplete', $eachPage)
          )
            $eachPage['lastVisualChange'] = $eachPage['visualComplete'];
        }
        // see if we need a custom Speed Index as well
        $end = null;
        if (isset($options) && array_key_exists('end', $options)) {
          $end = $options['end'];
          $progress = GetVisualProgress($testPath, $run, $cached, null, $end, $startOffset);
          if (isset($progress) && is_array($progress)) {
            if (array_key_exists('SpeedIndex', $progress))
              $eachPage['SpeedIndexCustom'] = $progress['SpeedIndex'];
            $eachPage['visualCompleteCustom'] = $progress['visualComplete'];
          }
        }

        if (isset($eachPage) && is_array($eachPage) && isset($eachPage['fullyLoaded']) && $eachPage['fullyLoaded']) {
          $processing = GetDevToolsCPUTime($testPath, $run, $cached, $eachPage['fullyLoaded']);
          if (isset($processing) && is_array($processing) && count($processing)) {
            $eachPage['cpuTimes'] = $processing;
            if (isset($eachPage['docTime']) && $eachPage['docTime']) {
              $processing = GetDevToolsCPUTime($testPath, $run, $cached, $eachPage['docTime']);
              if (isset($processing) && is_array($processing) && count($processing)) {
                $eachPage['cpuTimesDoc'] = $processing;
              }
            }
          }
        }
      }

      if (isset($eachPage)) {
        $eachPage['run'] = $run;
        $eachPage['cached'] = $cached;

        // calculate the effective bps
        if (array_key_exists('fullyLoaded', $eachPage) &&
            array_key_exists('TTFB', $eachPage) &&
            array_key_exists('bytesIn', $eachPage) &&
            $eachPage['fullyLoaded'] > 0 &&
            $eachPage['TTFB'] > 0 &&
            $eachPage['bytesIn'] > 0 &&
            $eachPage['fullyLoaded'] > $eachPage['TTFB']
        )
          $eachPage['effectiveBps'] = intval($eachPage['bytesIn'] / (($eachPage['fullyLoaded'] - $eachPage['TTFB']) / 1000.0));
        if (array_key_exists('docTime', $eachPage) &&
            array_key_exists('TTFB', $eachPage) &&
            array_key_exists('bytesInDoc', $eachPage) &&
            $eachPage['docTime'] > 0 &&
            $eachPage['TTFB'] > 0 &&
            $eachPage['bytesInDoc'] > 0 &&
            $eachPage['docTime'] > $eachPage['TTFB']
        )
          $eachPage['effectiveBpsDoc'] = intval($eachPage['bytesInDoc'] / (($eachPage['docTime'] - $eachPage['TTFB']) / 1000.0));
        // clean up any insane values (from negative numbers as unsigned most likely)
        if (array_key_exists('firstPaint', $eachPage) &&
            array_key_exists('fullyLoaded', $eachPage) &&
            $eachPage['firstPaint'] > $eachPage['fullyLoaded']
        )
          $eachPage['firstPaint'] = 0;
        $times = array('loadTime',
            'TTFB',
            'render',
            'fullyLoaded',
            'docTime',
            'domTime',
            'aft',
            'titleTime',
            'loadEventStart',
            'loadEventEnd',
            'domContentLoadedEventStart',
            'domContentLoadedEventEnd',
            'lastVisualChange',
            'server_rtt',
            'firstPaint');
        foreach ($times as $key) {
          if (!array_key_exists($key, $eachPage) ||
              $eachPage[$key] > 3600000 ||
              $eachPage[$key] < 0
          )
            $eachPage[$key] = 0;
        }
      }
    }
    return $ret;
}

/**
* Load the page data from the specified file
* 
* @param mixed $file
*/
// TODO(kk): should merge $multistep into $options.
function loadPageData($file, $options = null, $multistep=false)
{
    $ret = array();
    $lines = gz_file($file);
    if( $lines)
    {
        // loop through each line in the file until we get a data record
        foreach($lines as $linenum => $line) 
        {
            $parseLine = str_replace("\t", "\t ", $line);
            $fields = explode("\t", $parseLine);
            if( count($fields) > 34 && trim($fields[0]) != 'Date' )
            {
                $tmp = array(   'URL' => @htmlspecialchars(trim($fields[3])),
                                // 'loadTime' => (int)$fields[4],
                                'loadTime' => @(int)$fields[32],
                                'TTFB' => @(int)$fields[5],
                                'bytesOut' => @(int)$fields[7],
                                'bytesOutDoc' => @(int)$fields[45],
                                'bytesIn' => @(int)$fields[8],
                                'bytesInDoc' => @(int)$fields[46],
                                'connections' => @(int)$fields[10],
                                'requests' => @(int)$fields[11],
                                'requestsDoc' => @(int)$fields[49],
                                'responses_200' => @(int)$fields[12],
                                'responses_404' => @(int)$fields[15],
                                'responses_other' => @(int)$fields[16],
                                'result' => @(int)$fields[17],
                                'render' => @(int)$fields[18],
                                'fullyLoaded' => @(int)$fields[22],
                                'cached' => @(int)$fields[27],
                                'docTime' => @(int)$fields[32],
                                'domTime' => @(int)$fields[34],
                                'score_cache' => @(int)$fields[36],
                                'score_cdn' => @(int)$fields[37],
                                'score_gzip' => @(int)$fields[39],
                                'score_cookies' => @(int)$fields[40],
                                'score_keep-alive' => @(int)$fields[41],
                                'score_minify' => @(int)$fields[43],
                                'score_combine' => @(int)$fields[44],
                                'score_compress' => @(int)$fields[55],
                                'score_etags' => @(int)$fields[58],
                                'gzip_total' => @(int)$fields[64],
                                'gzip_savings' => @(int)$fields[65],
                                'minify_total' => @(int)$fields[66],
                                'minify_savings' => @(int)$fields[67],
                                'image_total' => @(int)$fields[68],
                                'image_savings' => @(int)$fields[69],
                                'optimization_checked' => @(int)$fields[71],
                                'aft' => @(int)$fields[72],
                                'domElements' => @(int)$fields[73],
                                'pageSpeedVersion' => @trim($fields[74]),
                                'title' => @htmlspecialchars(trim($fields[75]),ENT_NOQUOTES,'UTF-8'),
                                'titleTime' => @(int)$fields[76],
                                'loadEventStart' => @(int)$fields[77],
                                'loadEventEnd' => @(int)$fields[78],
                                'domContentLoadedEventStart' => @(int)$fields[79],
                                'domContentLoadedEventEnd' => @(int)$fields[80],
                                'lastVisualChange' => @(int)$fields[81],
                                'browser_name' => @trim($fields[82]),
                                'browser_version' => @trim($fields[83]),
                                'server_count' => @(int)trim($fields[84]),
                                'server_rtt' => @(int)trim($fields[85]),
                                'base_page_cdn' => @trim($fields[86]),
                                'adult_site' => @(int)trim($fields[87])
                            );

                $tmp['fixed_viewport'] = (array_key_exists(88, $fields) && strlen(trim($fields[88]))) ? (int)trim($fields[88]) : -1;
                $tmp['score_progressive_jpeg'] = (array_key_exists(89, $fields) && strlen(trim($fields[89]))) ? (int)trim($fields[89]) : -1;
                $tmp['firstPaint'] = (array_key_exists(90, $fields) && strlen(trim($fields[90]))) ? (int)trim($fields[90]) : 0;
                //$ret['peakMem'] = (array_key_exists(91, $fields) && strlen(trim($fields[91]))) ? (int)trim($fields[91]) : 0;
                //$ret['processCount'] = (array_key_exists(92, $fields) && strlen(trim($fields[92]))) ? (int)trim($fields[92]) : 0;
                $tmp['docCPUms'] = (array_key_exists(93, $fields) && strlen(trim($fields[93]))) ? floatval(trim($fields[93])) : 0.0;
                $tmp['fullyLoadedCPUms'] = (array_key_exists(94, $fields) && strlen(trim($fields[94]))) ? floatval(trim($fields[94])) : 0.0;
                $tmp['docCPUpct'] = (array_key_exists(95, $fields) && strlen(trim($fields[95]))) ? floatval(trim($fields[95])) : 0;
                $tmp['fullyLoadedCPUpct'] = (array_key_exists(96, $fields) && strlen(trim($fields[96]))) ? floatval(trim($fields[96])) : 0;
                $tmp['isResponsive'] = (array_key_exists(97, $fields) && strlen(trim($fields[97]))) ? intval(trim($fields[97])) : -1;
                
                $tmp['date'] = strtotime(trim($fields[0]) . ' ' . trim($fields[1]));
                if (!strlen($tmp['pageSpeedVersion']))
                    $tmp['pageSpeedVersion'] = '1.9';
                if ($multistep && strlen(trim($fields[2]))) {
                  $tmp['stepName'] = trim($fields[2]);
                  foreach ($tmp as $key => $value) {
                    $ret[$tmp['stepName']][$key] = $value;
                  }
                } else {
                  $ret = $tmp;
                  break;
                }
            }
        }
    }
    
    return count($ret) == 0 ? null : $ret;
}

/**
* Find the median run and use it for the results
* 
* @param mixed $pageData
*/
function calculatePageStats(&$pageData, &$fv, &$rv)
{
    $fvCount = 0;
    $rvCount = 0;
    
    // calculate the averages
    if( count($pageData) ) {
        foreach( $pageData as $run => $data ) {
            if( isset($data[0]) && $data[0]['cached'] === 0 ) {
                if (!isset($metrics)) {
                    $metrics = array();
                    foreach ($data[0] as $metric => $value)
                      if (is_numeric($value))
                        $metrics[] = $metric;
                }
                // only look at non-error runs
                if( successfulRun($data[0]) )
                {
                    if( !isset($fv) )
                        $fv = array();
                    foreach ($metrics as $metric) {
                      if (is_numeric($data[0][$metric])) {
                        if (array_key_exists($metric, $fv))
                            $fv[$metric] += $data[0][$metric];
                        else
                            $fv[$metric] = $data[0][$metric];
                      }
                    }
                    $fvCount++;
                }
            }

            if( isset($data[1]) && $data[1]['cached'] )
            {
                if (!isset($metrics)) {
                    $metrics = array();
                    foreach ($data[0] as $metric => $value)
                      if (is_numeric($value))
                        $metrics[] = $metric;
                }
                // only look at non-error runs
                if( successfulRun($data[1]) )
                {
                    if( !isset($rv) )
                        $rv = array();
                    foreach ($metrics as $metric) {
                      if (is_numeric($data[1][$metric])) {
                        if (array_key_exists($metric, $rv))
                            $rv[$metric] += $data[1][$metric];
                        else
                            $rv[$metric] = $data[1][$metric];
                      }
                    }
                    $rvCount++;
                }
            }
        }
    }

    // calculate the first view stats    
    if( isset($fv) && isset($metrics) && $fvCount > 0 )
    {
        foreach ($metrics as $metric)
          if (is_numeric($fv[$metric]))
            $fv[$metric] /= (double)$fvCount;
        
        // go through and find the run closest to the average
        $closest = -1;
        $distance = 10000000000;

        foreach( $pageData as $run => $data )
        {
            if( isset($data[0]) && successfulRun($data[0]) )
            {
                $curDist = abs($data[0]['loadTime'] - $fv['loadTime']);
                if( $curDist < $distance )
                {
                    $closest = $run;
                    $distance = $curDist;
                }
            }
        }
        
        if( $closest != -1 )
            $fv['avgRun'] = $closest;
    }

    // calculate the repeat view stats    
    if( isset($rv) && isset($metrics) && $rvCount > 0 )
    {
        foreach ($metrics as $metric)
          if (is_numeric($rv[$metric]))
            $rv[$metric] /= (double)$rvCount;
        
        // go through and find the run closest to the average
        $closest = -1;
        $distance = 10000000000;

        foreach( $pageData as $run => $data )
        {
            if( isset($data[1]) && successfulRun($data[1]) )
            {
                $curDist = abs($data[1]['loadTime'] - $rv['loadTime']);
                if( $curDist < $distance )
                {
                    $closest = $run;
                    $distance = $curDist;
                }
            }
        }
        
        if( $closest != -1 )
            $rv['avgRun'] = $closest;
    }
}

/**
 * Find the index of the test run in $pageData with cache status $cached
 * corresponding to the median (or lower of two middle values) of $metric,
 * unless the "medianRun" parameter is set to "fastest",
 * in which case it returns the index of the fastest run.
* 
* @param mixed $pageData
* @param mixed $cached
*/
function GetMedianRun(&$pageData, $cached, $metric = 'loadTime') {
    $run = 0;
    $cached = $cached ? 1:0;
    $times = values($pageData, $cached, $metric, true);
            
    if (!count($times)) {
      $times = values($pageData, $cached, $metric, false);
    }
    
    $count = count($times);
    if( $count > 1 ) {
        asort($times);
        if (array_key_exists('medianRun', $_REQUEST) &&
            $_REQUEST['medianRun'] == 'fastest')
          $medianIndex = 1;
        else
          $medianIndex = (int)floor(((float)$count + 1.0) / 2.0);
        $current = 0;
        foreach( $times as $index => $time ) {
            $current++;
            if( $current == $medianIndex ) {
                $run = $index;
                break;
            }
        }
    }
    elseif( $count == 1 ) {
        foreach( $times as $index => $time ) {
            $run = $index;
            break;
        }
    }
    
    // fall back to loadTime if we failed to get a run with the specified metric
    if (!$run && $metric != 'loadTime') {
        $run = GetMedianRun($pageData, $cached);
    }
    
    return $run;
}

/**
* Count the number of tests with successful results
* 
* @param mixed $pageData
* @param mixed $cached
*/
function CountSuccessfulTests(&$pageData, $cached)
{
    $count = 0;
    foreach( $pageData as &$run )
    {
        if( successfulRun($run[$cached]) )
            $count++;
    }
    
    return $count;
}

/**
* Calculate some stats for the given metric from the page data
* 
* @param mixed $pageData
* @param mixed $cached
* @param mixed $metric
* @param mixed $median
* @param mixed $avg
* @param mixed $stdDev
*/
function CalculateAggregateStats(&$pageData, $cached, $metric, &$median, &$avg, &$stdDev)
{
    $median = null;
    $avg = null;
    $stdDev = null;
    $count = 0;
    
    // first pass, calculate the average and array of values for grabbing the median
    $values = values($pageData, $cached, $metric, true);
    $sum = array_sum($values);
    $count = count($values);

    if( $count ) {
        $avg = $sum / $count;
        sort($values, SORT_NUMERIC);
        $medianIndex = (int)($count / 2);
        $median = $values[$medianIndex];
        
        // pass 2, calculate the standard deviation
        $sum = 0;
        foreach($values as $value){
            $sum += pow($value - $avg, 2);
        }
        $stdDev = sqrt($sum / $count);
    }
    
    return $count;
}

/**
* Calculate the standard deviation for the provided metric
* 
*/
function PageDataStandardDeviation($pageData, $metric, $cached) {
    $ret = null;
    $values = array();
    if( count($pageData) ) {
        foreach( $pageData as $run => $data ) {
            if( array_key_exists($cached, $data) &&
                array_key_exists($metric,$data[$cached]) &&
                array_key_exists('result', $data[$cached]) &&
                successfulRun($data[$cached]))
                $values[] = $data[$cached][$metric];
        }
    }
    $count = count($values);
    if ($count) {
        $sum = 0;
        foreach ($values as $value)
            $sum += $value;
        $avg = $sum / $count;
        $sum = 0;
        foreach ($values as $value)
            $sum += pow($value - $avg, 2);
        $ret = (int)sqrt($sum / $count);
    }
    return $ret;
}

/**
* Load the reported user timings data for the given run
* 
* @param mixed $testPath
* @param mixed $run
* @param mixed $cached
*/
function loadUserTimingData(&$pageData, $testPath, $run, $cached) {
  if( $cached )
    $file = "$testPath/{$run}_Cached_timed_events.json";
  else
    $file = "$testPath/{$run}_timed_events.json";
  if (gz_is_file($file)) {
    $events = json_decode(gz_file_get_contents($file), true);
    if (isset($events) && is_array($events) && count($events)) {
      $lastEvent = 0;
      foreach ($events as $event) {
        if (is_array($event) &&
            array_key_exists('name', $event) &&
            array_key_exists('startTime', $event)) {
          $name = preg_replace('/[^a-zA-Z0-9\.\-_\(\) ]/', '_', $event['name']);
          $time = intval($event['startTime'] + 0.5);
          if ($time > 0 && $time < 3600000) {
            if ($event['startTime'] > $lastEvent)
              $lastEvent = $event['startTime'];
            $pageData["userTime.$name"] = $time;
            if (!isset($pageData['userTimes']))
              $pageData['userTimes'] = array();
            $pageData['userTimes'][$name] = $time;
          }
        }
      }
      $pageData["userTime"] = intval($lastEvent + 0.5);
    }
  }
}

/**
 * Return whether a particular run (cached or uncached) was successful.
 *
 * @param mixed data
 *
 * @return bool
 */
function successfulRun($data) {
  return ($data['result'] === 0 || $data['result']  === 99999);
}

/**
 * Return all values from a pageData for a given cached state and metric
 *
 * @param mixed pageData
 * @param int cached
 * @param string metric
 * @param bool successfulOnly Whether to only include successful runs
 *
 * @return (int|float)[]
 */
function values(&$pageData, $cached, $metric, $successfulOnly) {
  $values = array();
  foreach( $pageData as $index => &$pageRun ) {
    if( array_key_exists($cached, $pageRun) &&
      (!$successfulOnly || successfulRun($pageRun[$cached])) &&
      array_key_exists($metric, $pageRun[$cached]) ) {
        $values[$index] = $pageRun[$cached][$metric];
    }
  }
  return $values;
}

?>
