#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import sys
from operator import add
import time

from pyspark.sql import SparkSession

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Missing text file input", file=sys.stderr)
        print("Usage: wordcount <file>", file=sys.stderr)
        sys.exit(-1)

    # Extract file argument
    filename = sys.argv[1]

    # Create spark session
    spark = SparkSession.builder.appName("WordCount").getOrCreate()
    spark.sparkContext.setLogLevel("WARN") # Reduce logging output
    
    # Time execution
    begin_time = time.time()

    # There are multiple ways read a textFile, the "old" way, used below using the `textFile`
    # method to load a RDD directly and a "new" method that first constructs a data.frame. The latter
    # is provided for reference, but is not needed to implement your program.
    lines = spark.sparkContext.textFile(filename)

    # Just for reference: The new SparkSession API returns a DataFrame when reading a text file,
    # lines = spark.read.text(filename).rdd.map(lambda r: r[0])

    # TODO: Implement word count, printing out the top-ten most common words in
    # descending order of count (and only ten), e.g.
    # the: 159
    # and: 145
    # ...

    end_time = time.time()
    print(f"Total program time: {(end_time-begin_time):.2f} seconds")

    spark.stop()
