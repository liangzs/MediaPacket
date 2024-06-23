pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "MediaPackage"
include(":app")
//include(":cameracature")
//include(":screenProjectPush")
//include(":commonLib")
//include(":screenprojectfetch")
//include(":videocall")
include(":audio")
include(":video")
include(":ffmplay")
