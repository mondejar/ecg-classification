from distutils.core import setup


with open('README') as f:
    long_description = f.read()


setup(
      name = 'py_dempster_shafer',
      version = '0.7',
      description = 'Dempster-Shafer theory library',
      author = 'Thomas Reineking',
      author_email = 'nikenier@gmail.com',
      py_modules = ['pyds'],
      url = 'https://github.com/reineking/pyds',
      long_description=long_description,
      keywords = 'Dempster-Shafer belief functions',
      classifiers = [
          'Development Status :: 5 - Production/Stable',
          'Topic :: Scientific/Engineering',
          'License :: OSI Approved :: BSD License',
          'Operating System :: OS Independent',
          'Programming Language :: Python'
      ],
)
