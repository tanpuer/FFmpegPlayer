package com.temple.ffmpegplayer

interface HYPlayerCallback {

    fun onPrepared(audioDuration: Long, videoDuration: Long, videoWidth: Int, videoHeight: Int)

    fun onProgressUpdate(currPos: Long)

    fun onComplete() {}

    fun onSeek(timeMills: Long) {}

}