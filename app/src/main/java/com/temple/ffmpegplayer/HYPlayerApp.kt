package com.temple.ffmpegplayer

import android.app.Application

class HYPlayerApp : Application() {

    override fun onCreate() {
        super.onCreate()
        mInstance = this
    }

    companion object {
        private lateinit var mInstance: HYPlayerApp

        fun getInstance(): HYPlayerApp {
            return mInstance
        }
    }
}