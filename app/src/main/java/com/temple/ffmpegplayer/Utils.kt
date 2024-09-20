package com.temple.ffmpegplayer

import java.util.Locale

object Utils {

    fun formatTime(timestamp: Long): String {
        val seconds = timestamp / 1000
        val minutes = seconds / 60
        val hours = minutes / 60

        val formattedTime = String.format(Locale.CHINA, "%02d:%02d", minutes % 60, seconds % 60)
        return if (hours > 0) {
            String.format(Locale.CHINA, "%02d:", hours) + formattedTime
        } else {
            formattedTime
        }
    }
}