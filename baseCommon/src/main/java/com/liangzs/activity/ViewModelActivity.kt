package com.liangzs.activity

import android.databinding.tool.writer.ViewBinding
import android.os.Bundle
import android.view.LayoutInflater
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import com.liangzs.base.BaseActivity

abstract class ViewModelActivity<VM : ViewModel, B : ViewBinding>(
    inflater: (inflater: LayoutInflater) -> B, val vmClass: Class<VM>
) : BaseActivity<B>(inflater) {

    lateinit var viewModel: VM;
    override fun onCreate(savedInstanceState: Bundle?) {
        viewModel = ViewModelProvider(this).get(vmClass)
        super.onCreate(savedInstanceState)
    }

}